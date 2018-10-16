/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Windows main function of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  This file contains the code in the qtmain library for WinRT.
  qtmain contains the WinRT startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked. This WinMain creates the WinRT application
  container, which in turn calls the application's main()
  entry point within the newly created GUI thread.
*/

extern "C" {
    int main(int, char **);
}

#include <qbytearray.h>
#include <qstring.h>
#include <qdir.h>
#include <qstandardpaths.h>
#include <qfunctions_winrt.h>
#include <qcoreapplication.h>
#include <qmutex.h>

#include <wrl.h>
#include <Windows.ApplicationModel.core.h>
#include <windows.ui.xaml.h>
#include <windows.ui.xaml.controls.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#define qHString(x) Wrappers::HString::MakeReference(x).Get()
#define CoreApplicationClass RuntimeClass_Windows_ApplicationModel_Core_CoreApplication
typedef ITypedEventHandler<CoreApplicationView *, Activation::IActivatedEventArgs *> ActivatedHandler;

const quint32 resizeMessageType = QtInfoMsg + 1;

const PCWSTR shmemName = L"qdebug-shmem";
const PCWSTR eventName = L"qdebug-event";
const PCWSTR ackEventName = L"qdebug-event-ack";

static QtMessageHandler defaultMessageHandler;
static void devMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    static HANDLE shmem = 0;
    static HANDLE event = 0;
    static HANDLE ackEvent = 0;

    static QMutex messageMutex;
    QMutexLocker locker(&messageMutex);

    static quint64 mappingSize = 4096;
    const quint32 copiedMessageLength = message.length() + 1;
    // Message format is message type + message. We need the message's length + 4 bytes for the type
    const quint64 copiedMessageSize = copiedMessageLength * sizeof(wchar_t) + sizeof(quint32);
    if (copiedMessageSize > mappingSize) {
        if (!shmem)
            shmem = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, mappingSize, shmemName);
        Q_ASSERT_X(shmem, Q_FUNC_INFO, "Could not create file mapping");

        quint32 *data = reinterpret_cast<quint32 *>(MapViewOfFileFromApp(shmem, FILE_MAP_WRITE, 0, mappingSize));
        Q_ASSERT_X(data, Q_FUNC_INFO, "Could not map size file");

        mappingSize = copiedMessageSize;

        memcpy(data, (void *)&resizeMessageType, sizeof(quint32));
        memcpy(data + 1, (void *)&mappingSize, sizeof(quint64));
        UnmapViewOfFile(data);
        SetEvent(event);
        WaitForSingleObjectEx(ackEvent, INFINITE, false);
        if (shmem) {
            if (!CloseHandle(shmem))
                Q_ASSERT_X(false, Q_FUNC_INFO, "Could not close shared file handle");
            shmem = 0;
        }
    }

    if (!shmem)
        shmem = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, mappingSize, shmemName);
    if (!event)
        event = CreateEventEx(NULL, eventName, 0, EVENT_ALL_ACCESS);
    if (!ackEvent)
        ackEvent = CreateEventEx(NULL, ackEventName, 0, EVENT_ALL_ACCESS);

    Q_ASSERT_X(shmem, Q_FUNC_INFO, "Could not create file mapping");
    Q_ASSERT_X(event, Q_FUNC_INFO, "Could not create debug event");

    void *data = MapViewOfFileFromApp(shmem, FILE_MAP_WRITE, 0, mappingSize);
    Q_ASSERT_X(data, Q_FUNC_INFO, "Could not map file");

    memset(data, quint32(type), sizeof(quint32));
    memcpy_s(static_cast<quint32 *>(data) + 1, mappingSize - sizeof(quint32),
             message.data(), copiedMessageLength * sizeof(wchar_t));
    UnmapViewOfFile(data);
    SetEvent(event);
    WaitForSingleObjectEx(ackEvent, INFINITE, false);
    locker.unlock();
    defaultMessageHandler(type, context, message);
}

class QActivationEvent : public QEvent
{
public:
    explicit QActivationEvent(IInspectable *args)
        : QEvent(QEvent::WinEventAct)
    {
        setAccepted(false);
        args->AddRef();
        d = reinterpret_cast<QEventPrivate *>(args);
    }

    ~QActivationEvent() {
        IUnknown *args = reinterpret_cast<IUnknown *>(d);
        args->Release();
        d = nullptr;
    }
};

class AppContainer : public RuntimeClass<Xaml::IApplicationOverrides>
{
public:
    AppContainer()
    {
        ComPtr<Xaml::IApplicationFactory> applicationFactory;
        HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Application).Get(),
                                            IID_PPV_ARGS(&applicationFactory));
        Q_ASSERT_SUCCEEDED(hr);

        hr = applicationFactory->CreateInstance(this, &base, &core);
        RETURN_VOID_IF_FAILED("Failed to create application container instance");

        pidFile = INVALID_HANDLE_VALUE;
    }

    ~AppContainer()
    {
    }

    int exec()
    {
        mainThread = CreateThread(NULL, 0, [](void *param) -> DWORD {
            AppContainer *app = reinterpret_cast<AppContainer *>(param);
            int argc = app->args.count() - 1;
            char **argv = app->args.data();
            const int res = main(argc, argv);
            if (app->pidFile != INVALID_HANDLE_VALUE) {
                const QByteArray resString = QByteArray::number(res);
                WriteFile(app->pidFile, reinterpret_cast<LPCVOID>(resString.constData()),
                          resString.size(), NULL, NULL);
                FlushFileBuffers(app->pidFile);
                CloseHandle(app->pidFile);
            }
            app->core->Exit();
            return res;
        }, this, CREATE_SUSPENDED, nullptr);
        Q_ASSERT_X(mainThread, Q_FUNC_INFO, "Could not create Qt main thread");

        HRESULT hr;
        ComPtr<Xaml::IApplicationStatics> appStatics;
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Application).Get(),
                                    IID_PPV_ARGS(&appStatics));
        Q_ASSERT_SUCCEEDED(hr);
        hr = appStatics->Start(Callback<Xaml::IApplicationInitializationCallback>([](Xaml::IApplicationInitializationCallbackParams *) {
            return S_OK;
        }).Get());
        Q_ASSERT_SUCCEEDED(hr);

        WaitForSingleObjectEx(mainThread, INFINITE, FALSE);
        DWORD exitCode;
        GetExitCodeThread(mainThread, &exitCode);
        return exitCode;
    }

private:
    HRESULT activatedLaunch(IInspectable *activateArgs) {
        // Check if an application instance is already running
        // This is mostly needed for Windows Phone and file pickers
        QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
        if (dispatcher) {
            QCoreApplication::postEvent(dispatcher, new QActivationEvent(activateArgs));
            return S_OK;
        }

        QCoreApplication *app = QCoreApplication::instance();

        // Check whether the app already runs
        if (!app) {
            // I*EventArgs have no launch arguments, hence we
            // need to prepend the application binary manually
            wchar_t fn[513];
            DWORD res = GetModuleFileName(0, fn, 512);

            if (SUCCEEDED(res))
                args.prepend(QString::fromWCharArray(fn, res).toUtf8().data());

            ResumeThread(mainThread);

            // We give main() a max of 100ms to create an application object.
            // No eventhandling needs to happen at that point, all we want is
            // append our activation event
            int iterations = 0;
            while (true) {
                app = QCoreApplication::instance();
                if (app || iterations++ > 10)
                    break;
                Sleep(10);
            }
        }

        if (app)
            QCoreApplication::postEvent(app, new QActivationEvent(activateArgs));
        return S_OK;
    }

    HRESULT __stdcall OnActivated(IActivatedEventArgs *args) override
    {
        return activatedLaunch(args);
    }

    HRESULT __stdcall OnLaunched(ILaunchActivatedEventArgs *launchArgs) override
    {
        ComPtr<IPrelaunchActivatedEventArgs> preArgs;
        HRESULT hr = launchArgs->QueryInterface(preArgs.GetAddressOf());
        if (SUCCEEDED(hr)) {
            boolean prelaunched;
            preArgs->get_PrelaunchActivated(&prelaunched);
            if (prelaunched)
                return S_OK;
        }

        commandLine = QString::fromWCharArray(GetCommandLine()).toUtf8();

        HString launchCommandLine;
        launchArgs->get_Arguments(launchCommandLine.GetAddressOf());
        if (launchCommandLine.IsValid()) {
            quint32 launchCommandLineLength;
            const wchar_t *launchCommandLineBuffer = launchCommandLine.GetRawBuffer(&launchCommandLineLength);
            if (!commandLine.isEmpty() && launchCommandLineLength)
                commandLine += ' ';
            if (launchCommandLineLength)
                commandLine += QString::fromWCharArray(launchCommandLineBuffer, launchCommandLineLength).toUtf8();
        }
        if (!commandLine.isEmpty())
            args.append(commandLine.data());

        bool quote = false;
        bool escape = false;
        for (int i = 0; i < commandLine.size(); ++i) {
            switch (commandLine.at(i)) {
            case '\\':
                escape = true;
                break;
            case '"':
                if (escape) {
                    escape = false;
                    break;
                }
                quote = !quote;
                commandLine[i] = '\0';
                break;
            case ' ':
                if (quote)
                    break;
                commandLine[i] = '\0';
                if (args.last()[0] != '\0')
                    args.append(commandLine.data() + i + 1);
                // fall through
            default:
                if (args.last()[0] == '\0')
                    args.last() = commandLine.data() + i;
                escape = false; // only quotes are escaped
                break;
            }
        }

        if (args.count() >= 2 && strncmp(args.at(1), "-ServerName:", 12) == 0)
            args.remove(1);

        bool develMode = false;
        bool debugWait = false;
        for (int i = args.count() - 1; i >= 0; --i) {
            const char *arg = args.at(i);
            if (strcmp(arg, "-qdevel") == 0) {
                develMode = true;
                args.remove(i);
            } else if (strcmp(arg, "-qdebug") == 0) {
                debugWait = true;
                args.remove(i);
            }
        }
        args.append(nullptr);

        if (develMode) {
            // Write a PID file to help runner
            const QString pidFileName = QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
                    .absoluteFilePath(QString::asprintf("%u.pid", uint(GetCurrentProcessId())));
            CREATEFILE2_EXTENDED_PARAMETERS params = {
                sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
                FILE_ATTRIBUTE_NORMAL
            };
            pidFile = CreateFile2(reinterpret_cast<LPCWSTR>(pidFileName.utf16()),
                        GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, &params);
            // Install the develMode message handler
            defaultMessageHandler = qInstallMessageHandler(devMessageHandler);
        }
        // Wait for debugger before continuing
        if (debugWait) {
            while (!IsDebuggerPresent())
                WaitForSingleObjectEx(GetCurrentThread(), 1, true);
        }

        ResumeThread(mainThread);
        return S_OK;
    }

    HRESULT __stdcall OnFileActivated(IFileActivatedEventArgs *args) override
    {
        return activatedLaunch(args);
    }

    HRESULT __stdcall OnSearchActivated(ISearchActivatedEventArgs *args) override
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnShareTargetActivated(IShareTargetActivatedEventArgs *args) override
    {
        return activatedLaunch(args);
    }

    HRESULT __stdcall OnFileOpenPickerActivated(IFileOpenPickerActivatedEventArgs *args) override
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnFileSavePickerActivated(IFileSavePickerActivatedEventArgs *args) override
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnCachedFileUpdaterActivated(ICachedFileUpdaterActivatedEventArgs *args) override
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnWindowCreated(Xaml::IWindowCreatedEventArgs *args) override
    {
        Q_UNUSED(args);
        return S_OK;
    }

    ComPtr<Xaml::IApplicationOverrides> base;
    ComPtr<Xaml::IApplication> core;
    QByteArray commandLine;
    QVarLengthArray<char *> args;
    HANDLE mainThread{0};
    HANDLE pidFile;
};

// Main entry point for Appx containers
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
        return 1;

    ComPtr<AppContainer> app = Make<AppContainer>();
    return app->exec();
}
