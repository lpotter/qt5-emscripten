/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
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

package org.qtproject.qt5.android;

import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

import android.app.Activity;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.content.ClipboardManager;
import android.content.ClipboardManager.OnPrimaryClipChangedListener;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MotionEvent;
import android.view.View;
import android.view.InputDevice;

import java.lang.reflect.Method;
import java.security.KeyStore;
import java.security.cert.X509Certificate;
import java.util.Iterator;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public class QtNative
{
    private static Activity m_activity = null;
    private static boolean m_activityPaused = false;
    private static Service m_service = null;
    private static QtActivityDelegate m_activityDelegate = null;
    private static QtServiceDelegate m_serviceDelegate = null;
    public static Object m_mainActivityMutex = new Object(); // mutex used to synchronize runnable operations

    public static final String QtTAG = "Qt JAVA"; // string used for Log.x
    private static ArrayList<Runnable> m_lostActions = new ArrayList<Runnable>(); // a list containing all actions which could not be performed (e.g. the main activity is destroyed, etc.)
    private static boolean m_started = false;
    private static int m_displayMetricsScreenWidthPixels = 0;
    private static int m_displayMetricsScreenHeightPixels = 0;
    private static int m_displayMetricsDesktopWidthPixels = 0;
    private static int m_displayMetricsDesktopHeightPixels = 0;
    private static double m_displayMetricsXDpi = .0;
    private static double m_displayMetricsYDpi = .0;
    private static double m_displayMetricsScaledDensity = 1.0;
    private static double m_displayMetricsDensity = 1.0;
    private static int m_oldx, m_oldy;
    private static final int m_moveThreshold = 0;
    private static ClipboardManager m_clipboardManager = null;
    private static Method m_checkSelfPermissionMethod = null;
    private static Boolean m_tabletEventSupported = null;
    public static QtThread m_qtThread = new QtThread();
    private static final Runnable runPendingCppRunnablesRunnable = new Runnable() {
        @Override
        public void run() {
            runPendingCppRunnables();
        }
    };

    private static ClassLoader m_classLoader = null;
    public static ClassLoader classLoader()
    {
        return m_classLoader;
    }

    public static void setClassLoader(ClassLoader classLoader)
    {
            m_classLoader = classLoader;
    }

    public static Activity activity()
    {
        synchronized (m_mainActivityMutex) {
            return m_activity;
        }
    }

    public static Service service()
    {
        synchronized (m_mainActivityMutex) {
            return m_service;
        }
    }


    public static QtActivityDelegate activityDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_activityDelegate;
        }
    }

    public static QtServiceDelegate serviceDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_serviceDelegate;
        }
    }

    public static boolean openURL(String url, String mime)
    {
        boolean ok = true;

        try {
            Uri uri = Uri.parse(url);
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            if (!mime.isEmpty())
                intent.setDataAndType(uri, mime);
            activity().startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
            ok = false;
        }

        return ok;
    }

    // this method loads full path libs
    public static void loadQtLibraries(final ArrayList<String> libraries)
    {
        m_qtThread.run(new Runnable() {
            @Override
            public void run() {
                if (libraries == null)
                    return;
                for (String libName : libraries) {
                    try {
                        File f = new File(libName);
                        if (f.exists())
                            System.load(libName);
                    } catch (SecurityException e) {
                        Log.i(QtTAG, "Can't load '" + libName + "'", e);
                    } catch (Exception e) {
                        Log.i(QtTAG, "Can't load '" + libName + "'", e);
                    }
                }
            }
        });
    }

    // this method loads bundled libs by name.
    public static void loadBundledLibraries(final ArrayList<String> libraries, final String nativeLibraryDir)
    {
        m_qtThread.run(new Runnable() {
            @Override
            public void run() {
                if (libraries == null)
                    return;

                for (String libName : libraries) {
                    try {
                        String libNameTemplate = "lib" + libName + ".so";
                        File f = new File(nativeLibraryDir + libNameTemplate);
                        if (!f.exists()) {
                            Log.i(QtTAG, "Can't find '" + f.getAbsolutePath());
                            try {
                                ActivityInfo info = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(),
                                                                                           PackageManager.GET_META_DATA);
                                String systemLibraryDir = QtNativeLibrariesDir.systemLibrariesDir;
                                if (info.metaData.containsKey("android.app.system_libs_prefix"))
                                    systemLibraryDir = info.metaData.getString("android.app.system_libs_prefix");
                                f = new File(systemLibraryDir + libNameTemplate);
                            } catch (Exception e) {

                            }
                        }
                        if (f.exists())
                            System.load(f.getAbsolutePath());
                        else
                            Log.i(QtTAG, "Can't find '" + f.getAbsolutePath());
                    } catch (Exception e) {
                        Log.i(QtTAG, "Can't load '" + libName + "'", e);
                    }
                }
            }
        });
    }

    public static void setActivity(Activity qtMainActivity, QtActivityDelegate qtActivityDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_activity = qtMainActivity;
            m_activityDelegate = qtActivityDelegate;
        }
    }

    public static void setService(Service qtMainService, QtServiceDelegate qtServiceDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_service = qtMainService;
            m_serviceDelegate = qtServiceDelegate;
        }
    }

    public static void setApplicationState(int state)
    {
        synchronized (m_mainActivityMutex) {
            switch (state) {
                case QtActivityDelegate.ApplicationActive:
                    m_activityPaused = false;
                    Iterator<Runnable> itr = m_lostActions.iterator();
                    while (itr.hasNext())
                        runAction(itr.next());
                    m_lostActions.clear();
                    break;
                default:
                    m_activityPaused = true;
                    break;
            }
        }
        updateApplicationState(state);
    }

    private static void runAction(Runnable action)
    {
        synchronized (m_mainActivityMutex) {
            final Looper mainLooper = Looper.getMainLooper();
            final Handler handler = new Handler(mainLooper);
            final boolean actionIsQueued = !m_activityPaused && m_activity != null && mainLooper != null && handler.post(action);
            if (!actionIsQueued)
                m_lostActions.add(action);
        }
    }

    private static void runPendingCppRunnablesOnAndroidThread()
    {
        synchronized (m_mainActivityMutex) {
            if (m_activity != null) {
                if (!m_activityPaused)
                    m_activity.runOnUiThread(runPendingCppRunnablesRunnable);
                else
                    runAction(runPendingCppRunnablesRunnable);
            } else {
                final Looper mainLooper = Looper.getMainLooper();
                final Thread looperThread = mainLooper.getThread();
                if (looperThread.equals(Thread.currentThread())) {
                    runPendingCppRunnablesRunnable.run();
                } else {
                    final Handler handler = new Handler(mainLooper);
                    handler.post(runPendingCppRunnablesRunnable);
                }
            }
        }
    }

    private static void setViewVisibility(final View view, final boolean visible)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                view.setVisibility(visible ? View.VISIBLE : View.GONE);
            }
        });
    }

    public static boolean startApplication(String params,
                                           final String environment,
                                           String mainLibrary,
                                           String nativeLibraryDir) throws Exception
    {
        String mainLibNameTemplate = "lib" + mainLibrary + ".so";
        File f = new File(nativeLibraryDir + mainLibNameTemplate);
        if (!f.exists()) {
            try {
                ActivityInfo info = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(),
                                                                                   PackageManager.GET_META_DATA);
                String systemLibraryDir = QtNativeLibrariesDir.systemLibrariesDir;
                if (info.metaData.containsKey("android.app.system_libs_prefix"))
                    systemLibraryDir = info.metaData.getString("android.app.system_libs_prefix");
                f = new File(systemLibraryDir + mainLibNameTemplate);
            } catch (Exception e) {

            }
        }
        if (!f.exists())
            throw new Exception("Can't find main library '" + mainLibrary + "'");

        if (params == null)
            params = "-platform\tandroid";

        final String mainLibraryPath = f.getAbsolutePath();
        final boolean[] res = new boolean[1];
        res[0] = false;
        synchronized (m_mainActivityMutex) {
            if (params.length() > 0 && !params.startsWith("\t"))
                params = "\t" + params;
            final String qtParams = f.getAbsolutePath() + params;
            m_qtThread.run(new Runnable() {
                @Override
                public void run() {
                    try {
                        System.load(mainLibraryPath);
                    } catch (Exception e) {
                        Log.i(QtTAG, "Can't load '" + mainLibraryPath + "'", e);
                    }
                    res[0] = startQtAndroidPlugin(qtParams, environment);
                    setDisplayMetrics(m_displayMetricsScreenWidthPixels,
                                      m_displayMetricsScreenHeightPixels,
                                      m_displayMetricsDesktopWidthPixels,
                                      m_displayMetricsDesktopHeightPixels,
                                      m_displayMetricsXDpi,
                                      m_displayMetricsYDpi,
                                      m_displayMetricsScaledDensity,
                                      m_displayMetricsDensity);
                }
            });
            m_qtThread.post(new Runnable() {
                @Override
                public void run() {
                    startQtApplication();
                }
            });
            waitForServiceSetup();
            m_started = true;
        }
        return res[0];
    }

    public static void setApplicationDisplayMetrics(int screenWidthPixels,
                                                    int screenHeightPixels,
                                                    int desktopWidthPixels,
                                                    int desktopHeightPixels,
                                                    double XDpi,
                                                    double YDpi,
                                                    double scaledDensity,
                                                    double density)
    {
        /* Fix buggy dpi report */
        if (XDpi < android.util.DisplayMetrics.DENSITY_LOW)
            XDpi = android.util.DisplayMetrics.DENSITY_LOW;
        if (YDpi < android.util.DisplayMetrics.DENSITY_LOW)
            YDpi = android.util.DisplayMetrics.DENSITY_LOW;

        synchronized (m_mainActivityMutex) {
            if (m_started) {
                setDisplayMetrics(screenWidthPixels,
                                  screenHeightPixels,
                                  desktopWidthPixels,
                                  desktopHeightPixels,
                                  XDpi,
                                  YDpi,
                                  scaledDensity,
                                  density);
            } else {
                m_displayMetricsScreenWidthPixels = screenWidthPixels;
                m_displayMetricsScreenHeightPixels = screenHeightPixels;
                m_displayMetricsDesktopWidthPixels = desktopWidthPixels;
                m_displayMetricsDesktopHeightPixels = desktopHeightPixels;
                m_displayMetricsXDpi = XDpi;
                m_displayMetricsYDpi = YDpi;
                m_displayMetricsScaledDensity = scaledDensity;
                m_displayMetricsDensity = density;
            }
        }
    }



    // application methods
    public static native boolean startQtAndroidPlugin(String params, String env);
    public static native void startQtApplication();
    public static native void waitForServiceSetup();
    public static native void quitQtCoreApplication();
    public static native void quitQtAndroidPlugin();
    public static native void terminateQt();
    // application methods

    private static void quitApp()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                quitQtAndroidPlugin();
                if (m_activity != null)
                     m_activity.finish();
                 if (m_service != null)
                     m_service.stopSelf();
            }
        });
    }

    //@ANDROID-9
    static private int getAction(int index, MotionEvent event)
    {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE) {
            int hsz = event.getHistorySize();
            if (hsz > 0) {
                float x = event.getX(index);
                float y = event.getY(index);
                for (int h = 0; h < hsz; ++h) {
                    if ( event.getHistoricalX(index, h) != x ||
                         event.getHistoricalY(index, h) != y )
                        return 1;
                }
                return 2;
            }
            return 1;
        }
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN && index == event.getActionIndex()) {
            return 0;
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP && index == event.getActionIndex()) {
            return 3;
        }
        return 2;
    }
    //@ANDROID-9

    static public void sendTouchEvent(MotionEvent event, int id)
    {
        int pointerType = 0;

        if (m_tabletEventSupported == null)
            m_tabletEventSupported = isTabletEventSupported();

        switch (event.getToolType(0)) {
        case MotionEvent.TOOL_TYPE_STYLUS:
            pointerType = 1; // QTabletEvent::Pen
            break;
        case MotionEvent.TOOL_TYPE_ERASER:
            pointerType = 3; // QTabletEvent::Eraser
            break;
        }

        if (event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
            sendMouseEvent(event, id);
        } else if (m_tabletEventSupported && pointerType != 0) {
            tabletEvent(id, event.getDeviceId(), event.getEventTime(), event.getAction(), pointerType,
                event.getButtonState(), event.getX(), event.getY(), event.getPressure());
        } else {
            touchBegin(id);
            for (int i = 0; i < event.getPointerCount(); ++i) {
                    touchAdd(id,
                             event.getPointerId(i),
                             getAction(i, event),
                             i == 0,
                             (int)event.getX(i),
                             (int)event.getY(i),
                             event.getTouchMajor(i),
                             event.getTouchMinor(i),
                             event.getOrientation(i),
                             event.getPressure(i));
            }

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    touchEnd(id, 0);
                    break;

                case MotionEvent.ACTION_UP:
                    touchEnd(id, 2);
                    break;

                default:
                    touchEnd(id, 1);
            }
        }
    }

    static public void sendTrackballEvent(MotionEvent event, int id)
    {
        sendMouseEvent(event,id);
    }

    static public boolean sendGenericMotionEvent(MotionEvent event, int id)
    {
        if (((event.getAction() & (MotionEvent.ACTION_SCROLL | MotionEvent.ACTION_HOVER_MOVE)) == 0)
                || (event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != InputDevice.SOURCE_CLASS_POINTER) {
            return false;
        }

        return sendMouseEvent(event, id);
    }

    static public boolean sendMouseEvent(MotionEvent event, int id)
    {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_UP:
                mouseUp(id, (int) event.getX(), (int) event.getY());
                break;

            case MotionEvent.ACTION_DOWN:
                mouseDown(id, (int) event.getX(), (int) event.getY());
                m_oldx = (int) event.getX();
                m_oldy = (int) event.getY();
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
            case MotionEvent.ACTION_MOVE:
                if (event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
                    mouseMove(id, (int) event.getX(), (int) event.getY());
                } else {
                    int dx = (int) (event.getX() - m_oldx);
                    int dy = (int) (event.getY() - m_oldy);
                    if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        mouseMove(id, (int) event.getX(), (int) event.getY());
                        m_oldx = (int) event.getX();
                        m_oldy = (int) event.getY();
                    }
                }
                break;
            case MotionEvent.ACTION_SCROLL:
                mouseWheel(id, (int) event.getX(), (int) event.getY(),
                        event.getAxisValue(MotionEvent.AXIS_HSCROLL), event.getAxisValue(MotionEvent.AXIS_VSCROLL));
                break;
            default:
                return false;
        }
        return true;
    }

    public static Context getContext() {
        if (m_activity != null)
            return m_activity;
        return m_service;
    }

    public static int checkSelfPermission(String permission)
    {
        int perm = PackageManager.PERMISSION_DENIED;
        synchronized (m_mainActivityMutex) {
            Context context = getContext();
            try {
                if (Build.VERSION.SDK_INT >= 23) {
                    if (m_checkSelfPermissionMethod == null)
                        m_checkSelfPermissionMethod = Context.class.getMethod("checkSelfPermission", String.class);
                    perm = (Integer)m_checkSelfPermissionMethod.invoke(context, permission);
                } else {
                    final PackageManager pm = context.getPackageManager();
                    perm = pm.checkPermission(permission, context.getApplicationContext().getPackageName());
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        return perm;
    }

    private static void updateSelection(final int selStart,
                                        final int selEnd,
                                        final int candidatesStart,
                                        final int candidatesEnd)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.updateSelection(selStart, selEnd, candidatesStart, candidatesEnd);
            }
        });
    }

    private static void updateHandles(final int mode,
                                      final int editX,
                                      final int editY,
                                      final int editButtons,
                                      final int x1,
                                      final int y1,
                                      final int x2,
                                      final int y2,
                                      final boolean rtl)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.updateHandles(mode, editX, editY, editButtons, x1, y1, x2, y2, rtl);
            }
        });
    }

    private static void showSoftwareKeyboard(final int x,
                                             final int y,
                                             final int width,
                                             final int height,
                                             final int inputHints,
                                             final int enterKeyType)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.showSoftwareKeyboard(x, y, width, height, inputHints, enterKeyType);
            }
        });
    }

    private static void resetSoftwareKeyboard()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.resetSoftwareKeyboard();
            }
        });
    }

    private static void hideSoftwareKeyboard()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.hideSoftwareKeyboard();
            }
        });
    }

    private static void setFullScreen(final boolean fullScreen)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.setFullScreen(fullScreen);
                }
                updateWindow();
            }
        });
    }

    private static void registerClipboardManager()
    {
        if (m_service == null || m_activity != null) { // Avoid freezing if only service
            final Semaphore semaphore = new Semaphore(0);
            runAction(new Runnable() {
                @Override
                public void run() {
                    if (m_activity != null)
                        m_clipboardManager = (android.content.ClipboardManager) m_activity.getSystemService(Context.CLIPBOARD_SERVICE);
                    if (m_clipboardManager != null) {
                        m_clipboardManager.addPrimaryClipChangedListener(new ClipboardManager.OnPrimaryClipChangedListener() {
                            public void onPrimaryClipChanged() {
                                onClipboardDataChanged();
                            }
                        });
                    }
                    semaphore.release();
                }
            });
            try {
                semaphore.acquire();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private static void setClipboardText(String text)
    {
        if (m_clipboardManager != null)
            m_clipboardManager.setText(text);
    }

    public static boolean hasClipboardText()
    {
        if (m_clipboardManager != null)
            return m_clipboardManager.hasText();
        else
            return false;
    }

    private static String getClipboardText()
    {
        if (m_clipboardManager != null)
            return m_clipboardManager.getText().toString();
        else
            return "";
    }

    private static void openContextMenu(final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.openContextMenu(x, y, w, h);
            }
        });
    }

    private static void closeContextMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.closeContextMenu();
            }
        });
    }

    private static void resetOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.resetOptionsMenu();
            }
        });
    }

    private static void openOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activity != null)
                    m_activity.openOptionsMenu();
            }
        });
    }

    private static byte[][] getSSLCertificates()
    {
        ArrayList<byte[]> certificateList = new ArrayList<byte[]>();

        try {
            TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);

            for (TrustManager manager : factory.getTrustManagers()) {
                if (manager instanceof X509TrustManager) {
                    X509TrustManager trustManager = (X509TrustManager) manager;

                    for (X509Certificate certificate : trustManager.getAcceptedIssuers()) {
                        byte buffer[] = certificate.getEncoded();
                        certificateList.add(buffer);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get certificates", e);
        }

        byte[][] certificateArray = new byte[certificateList.size()][];
        certificateArray = certificateList.toArray(certificateArray);
        return certificateArray;
    }

    private static void createSurface(final int id, final boolean onTop, final int x, final int y, final int w, final int h, final int imageDepth)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.createSurface(id, onTop, x, y, w, h, imageDepth);
            }
        });
    }

    private static void insertNativeView(final int id, final View view, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.insertNativeView(id, view, x, y, w, h);
            }
        });
    }

    private static void setSurfaceGeometry(final int id, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.setSurfaceGeometry(id, x, y, w, h);
            }
        });
    }

    private static void bringChildToFront(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.bringChildToFront(id);
            }
        });
    }

    private static void bringChildToBack(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.bringChildToBack(id);
            }
        });
    }

    private static void destroySurface(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.destroySurface(id);
            }
        });
    }

    private static void initializeAccessibility()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.initializeAccessibility();
            }
        });
    }

    private static void hideSplashScreen(final int duration)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.hideSplashScreen(duration);
            }
        });
    }

    // screen methods
    public static native void setDisplayMetrics(int screenWidthPixels,
                                                int screenHeightPixels,
                                                int desktopWidthPixels,
                                                int desktopHeightPixels,
                                                double XDpi,
                                                double YDpi,
                                                double scaledDensity,
                                                double density);
    public static native void handleOrientationChanged(int newRotation, int nativeOrientation);
    // screen methods

    // pointer methods
    public static native void mouseDown(int winId, int x, int y);
    public static native void mouseUp(int winId, int x, int y);
    public static native void mouseMove(int winId, int x, int y);
    public static native void mouseWheel(int winId, int x, int y, float hdelta, float vdelta);
    public static native void touchBegin(int winId);
    public static native void touchAdd(int winId, int pointerId, int action, boolean primary, int x, int y, float major, float minor, float rotation, float pressure);
    public static native void touchEnd(int winId, int action);
    public static native void longPress(int winId, int x, int y);
    // pointer methods

    // tablet methods
    public static native boolean isTabletEventSupported();
    public static native void tabletEvent(int winId, int deviceId, long time, int action, int pointerType, int buttonState, float x, float y, float pressure);
    // tablet methods

    // keyboard methods
    public static native void keyDown(int key, int unicode, int modifier, boolean autoRepeat);
    public static native void keyUp(int key, int unicode, int modifier, boolean autoRepeat);
    public static native void keyboardVisibilityChanged(boolean visibility);
    public static native void keyboardGeometryChanged(int x, int y, int width, int height);
    // keyboard methods

    // handle methods
    public static final int IdCursorHandle = 1;
    public static final int IdLeftHandle = 2;
    public static final int IdRightHandle = 3;
    public static native void handleLocationChanged(int id, int x, int y);
    // handle methods

    // dispatch events methods
    public static native boolean dispatchGenericMotionEvent(MotionEvent ev);
    public static native boolean dispatchKeyEvent(KeyEvent event);
    // dispatch events methods

    // surface methods
    public static native void setSurface(int id, Object surface, int w, int h);
    // surface methods

    // window methods
    public static native void updateWindow();
    // window methods

    // application methods
    public static native void updateApplicationState(int state);

    // menu methods
    public static native boolean onPrepareOptionsMenu(Menu menu);
    public static native boolean onOptionsItemSelected(int itemId, boolean checked);
    public static native void onOptionsMenuClosed(Menu menu);

    public static native void onCreateContextMenu(ContextMenu menu);
    public static native void fillContextMenu(Menu menu);
    public static native boolean onContextItemSelected(int itemId, boolean checked);
    public static native void onContextMenuClosed(Menu menu);
    // menu methods

    // clipboard methods
    public static native void onClipboardDataChanged();
    // clipboard methods

    // activity methods
    public static native void onActivityResult(int requestCode, int resultCode, Intent data);
    public static native void onNewIntent(Intent data);

    public static native void runPendingCppRunnables();

    public static native void sendRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults);

    private static native void setNativeActivity(Activity activity);
    private static native void setNativeService(Service service);
    // activity methods

    // service methods
    public static native IBinder onBind(Intent intent);
    // service methods
}
