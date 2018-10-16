/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtmessagedialoghelper.h"
#include "qwinrttheme.h"

#include <QtGui/QTextDocument>
#include <QtCore/qfunctions_winrt.h>
#include <private/qeventdispatcher_winrt_p.h>

#include <functional>
#include <windows.ui.popups.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <wrl.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::UI::Popups;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

typedef IAsyncOperationCompletedHandler<IUICommand *> DialogCompletedHandler;

QT_BEGIN_NAMESPACE

class CommandId : public RuntimeClass<IInspectable>
{
public:
    CommandId(QPlatformDialogHelper::StandardButton button)
        : button(button) { }
    QPlatformDialogHelper::StandardButton button;
};

class QWinRTMessageDialogHelperPrivate
{
public:
    const QWinRTTheme *theme;
    bool shown;
    ComPtr<IAsyncInfo> info;
    QEventLoop loop;
};

QWinRTMessageDialogHelper::QWinRTMessageDialogHelper(const QWinRTTheme *theme)
    : QPlatformMessageDialogHelper(), d_ptr(new QWinRTMessageDialogHelperPrivate)
{
    Q_D(QWinRTMessageDialogHelper);

    d->theme = theme;
    d->shown = false;
}

QWinRTMessageDialogHelper::~QWinRTMessageDialogHelper()
{
    Q_D(QWinRTMessageDialogHelper);

    if (d->shown)
        hide();
}

void QWinRTMessageDialogHelper::exec()
{
    Q_D(QWinRTMessageDialogHelper);

    if (!d->shown)
        show(Qt::Dialog, Qt::ApplicationModal, nullptr);
    d->loop.exec();
}

bool QWinRTMessageDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)
    Q_UNUSED(parent)
    Q_D(QWinRTMessageDialogHelper);

    QSharedPointer<QMessageDialogOptions> options = this->options();
    if (!options.data())
        return false;

    const QString informativeText = options->informativeText();
    const QString title = options->windowTitle();
    const QString text = informativeText.isEmpty() ? options->text() : (options->text() + QLatin1Char('\n') + informativeText);
    if (Qt::mightBeRichText(text)) {
        qWarning("Rich text detected, defaulting to QtWidgets-based dialog.");
        return false;
    }

    HRESULT hr;
    ComPtr<IMessageDialogFactory> dialogFactory;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Popups_MessageDialog).Get(),
                                IID_PPV_ARGS(&dialogFactory));
    RETURN_FALSE_IF_FAILED("Failed to create dialog factory");

    ComPtr<IUICommandFactory> commandFactory;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Popups_UICommand).Get(),
                                IID_PPV_ARGS(&commandFactory));
    RETURN_FALSE_IF_FAILED("Failed to create command factory");

    ComPtr<IMessageDialog> dialog;
    HStringReference nativeText(reinterpret_cast<LPCWSTR>(text.utf16()),
                                uint(text.size()));
    if (!title.isEmpty()) {
        HStringReference nativeTitle(reinterpret_cast<LPCWSTR>(title.utf16()),
                                     uint(title.size()));
        hr = dialogFactory->CreateWithTitle(nativeText.Get(), nativeTitle.Get(), &dialog);
        RETURN_FALSE_IF_FAILED("Failed to create dialog with title");
    } else {
        hr = dialogFactory->Create(nativeText.Get(), &dialog);
        RETURN_FALSE_IF_FAILED("Failed to create dialog");
    }

    hr = QEventDispatcherWinRT::runOnXamlThread([this, d, options, commandFactory, dialog]() {
        HRESULT hr;

        // Add Buttons
        ComPtr<IVector<IUICommand *>> dialogCommands;
        hr = dialog->get_Commands(&dialogCommands);
        RETURN_HR_IF_FAILED("Failed to get dialog commands");

        // If no button is specified we need to create one to get close notification
        int buttons = options->standardButtons();
        if (buttons == 0)
            buttons = Ok;

        for (int i = FirstButton; i < LastButton; i<<=1) {
            if (!(buttons & i))
                continue;
            // Add native command
            const QString label = d->theme->standardButtonText(i);
            HStringReference nativeLabel(reinterpret_cast<LPCWSTR>(label.utf16()),
                                         uint(label.size()));
            ComPtr<IUICommand> command;
            hr = commandFactory->Create(nativeLabel.Get(), &command);
            RETURN_HR_IF_FAILED("Failed to create message box command");
            ComPtr<IInspectable> id = Make<CommandId>(static_cast<StandardButton>(i));
            hr = command->put_Id(id.Get());
            RETURN_HR_IF_FAILED("Failed to set command ID");
            hr = dialogCommands->Append(command.Get());
            if (hr == E_BOUNDS) {
                qErrnoWarning(hr, "The WinRT message dialog supports a maximum of three buttons");
                continue;
            }
            RETURN_HR_IF_FAILED("Failed to append message box command");
            if (i == Abort || i == Cancel || i == Close) {
                quint32 size;
                hr = dialogCommands->get_Size(&size);
                RETURN_HR_IF_FAILED("Failed to get command list size");
                hr = dialog->put_CancelCommandIndex(size - 1);
                RETURN_HR_IF_FAILED("Failed to set cancel index");
            }
        }

        ComPtr<IAsyncOperation<IUICommand *>> op;
        hr = dialog->ShowAsync(&op);
        RETURN_HR_IF_FAILED("Failed to show dialog");
        hr = op->put_Completed(Callback<DialogCompletedHandler>(this, &QWinRTMessageDialogHelper::onCompleted).Get());
        RETURN_HR_IF_FAILED("Failed to set dialog callback");
        d->shown = true;
        hr = op.As(&d->info);
        RETURN_HR_IF_FAILED("Failed to acquire AsyncInfo for MessageDialog");
        return hr;
    });

    RETURN_FALSE_IF_FAILED("Failed to show dialog")
    return true;
}

void QWinRTMessageDialogHelper::hide()
{
    Q_D(QWinRTMessageDialogHelper);

    if (!d->shown)
        return;

    HRESULT hr = d->info->Cancel();
    if (FAILED(hr))
        qErrnoWarning(hr, "Failed to cancel dialog operation");

    d->shown = false;
}

HRESULT QWinRTMessageDialogHelper::onCompleted(IAsyncOperation<IUICommand *> *asyncInfo, AsyncStatus status)
{
    Q_UNUSED(status);
    Q_D(QWinRTMessageDialogHelper);

    QEventLoopLocker locker(&d->loop);

    d->shown = false;

    if (status == Canceled) {
        emit reject();
        return S_OK;
    }

    HRESULT hr;
    ComPtr<IUICommand> command;
    hr = asyncInfo->GetResults(&command);
    RETURN_OK_IF_FAILED("Failed to get command");

    ComPtr<CommandId> id;
    hr = command->get_Id(&id);
    RETURN_OK_IF_FAILED("Failed to get command ID");

    ButtonRole role = buttonRole(id->button);
    emit clicked(id->button, role);
    return S_OK;
}

QT_END_NAMESPACE
