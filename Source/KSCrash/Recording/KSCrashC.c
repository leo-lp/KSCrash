//
//  KSCrashC.c
//
//  Created by Karl Stenerud on 2012-01-28.
//
//  Copyright (c) 2012 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#include "KSCrashC.h"

#include "KSCrashReport.h"
#include "KSCrashReportStore.h"
#include "KSCrashMonitor_Deadlock.h"
#include "KSCrashMonitor_User.h"
#include "KSFileUtils.h"
#include "KSObjC.h"
#include "KSString.h"
#include "KSCrashMonitor_System.h"
#include "KSCrashMonitor_Zombie.h"
#include "KSCrashMonitor_AppState.h"
#include "KSCrashMonitorContext.h"

//#define KSLogger_LocalLevel TRACE
#include "KSLogger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// ============================================================================
#pragma mark - Globals -
// ============================================================================

/** True if KSCrash has been installed. */
static volatile bool g_installed = 0;

static bool g_redirectToLogFile = false;
static char* g_logFilePath;
static KSCrashMonitorType g_monitoring = KSCrashMonitorTypeProductionSafeMinimal;
static char g_lastCrashReportFilePath[KSFU_MAX_PATH_LENGTH];


// ============================================================================
#pragma mark - Callbacks -
// ============================================================================

/** Called when a crash occurs.
 *
 * This function gets passed as a callback to a crash handler.
 */
static void onCrash(struct KSCrash_MonitorContext* monitorContext)
{
    KSLOG_DEBUG("Updating application state to note crash.");
    kscrashstate_notifyAppCrash();


    if(monitorContext->crashedDuringCrashHandling)
    {
        kscrashreport_writeRecrashReport(monitorContext, g_lastCrashReportFilePath);
    }
    else
    {
        char crashReportFilePath[KSFU_MAX_PATH_LENGTH];
        kscrs_getNextCrashReportPath(crashReportFilePath);
        strncpy(g_lastCrashReportFilePath, crashReportFilePath, sizeof(g_lastCrashReportFilePath));
        kscrashreport_writeStandardReport(monitorContext, crashReportFilePath);
    }
}


// ============================================================================
#pragma mark - API -
// ============================================================================

KSCrashMonitorType kscrash_install(const char* appName, const char* const installPath)
{
    KSLOG_DEBUG("Installing crash reporter.");

    if(g_installed)
    {
        KSLOG_DEBUG("Crash reporter already installed.");
        return g_monitoring;
    }
    g_installed = 1;

    char path[KSFU_MAX_PATH_LENGTH];
    snprintf(path, sizeof(path), "%s/Reports", installPath);
    ksfu_makePath(path);
    kscrs_initialize(appName, path);

    snprintf(path, sizeof(path), "%s/Data", installPath);
    ksfu_makePath(path);
    snprintf(path, sizeof(path), "%s/Data/CrashState.json", installPath);
    kscrashstate_initialize(path);

    snprintf(path, sizeof(path), "%s/Data/ConsoleLog.txt", installPath);
    g_logFilePath = strdup(path);
    // Ensure this is set properly.
    kscrash_setRedirectConsoleLogToFile(g_redirectToLogFile);

    kscm_setEventCallback(onCrash);
    KSCrashMonitorType monitors = kscrash_setMonitoring(g_monitoring);

    KSLOG_DEBUG("Installation complete.");
    return monitors;
}

KSCrashMonitorType kscrash_setMonitoring(KSCrashMonitorType monitors)
{
    g_monitoring = monitors;
    
    if(g_installed)
    {
        kscm_setActiveMonitors(monitors);
        return kscm_getActiveMonitors();
    }
    // Return what we will be monitoring in future.
    return g_monitoring;
}

void kscrash_setUserInfoJSON(const char* const userInfoJSON)
{
    kscrashreport_setUserInfoJSON(userInfoJSON);
}

void kscrash_setDeadlockWatchdogInterval(double deadlockWatchdogInterval)
{
    kscm_setDeadlockHandlerWatchdogInterval(deadlockWatchdogInterval);
}

void kscrash_setPrintTraceToStdout(bool printTraceToStdout)
{
    kscrashreport_setPrintTraceToStdout(printTraceToStdout);
}

void kscrash_setSearchThreadNames(bool shouldSearchThreadNames)
{
    kscrashreport_setSearchThreadNames(shouldSearchThreadNames);
}

void kscrash_setSearchQueueNames(bool shouldSearchQueueNames)
{
    kscrashreport_setSearchQueueNames(shouldSearchQueueNames);
}

void kscrash_setIntrospectMemory(bool introspectMemory)
{
    kscrashreport_setIntrospectMemory(introspectMemory);
}

void kscrash_setDoNotIntrospectClasses(const char** doNotIntrospectClasses, int length)
{
    kscrashreport_setDoNotIntrospectClasses(doNotIntrospectClasses, length);
}

void kscrash_setCrashNotifyCallback(const KSReportWriteCallback onCrashNotify)
{
    kscrashreport_setUserSectionWriteCallback(onCrashNotify);
}

void kscrash_setRedirectConsoleLogToFile(bool shouldRedirectToFile)
{
    g_redirectToLogFile = shouldRedirectToFile;
    char* path = shouldRedirectToFile ? g_logFilePath : NULL;
    kslog_setLogFilename(path, true);
}

void kscrash_reportUserException(const char* name,
                                 const char* reason,
                                 const char* language,
                                 const char* lineOfCode,
                                 const char* stackTrace,
                                 bool logAllThreads,
                                 bool terminateProgram)
{
    kscm_reportUserException(name,
                             reason,
                             language,
                             lineOfCode,
                             stackTrace,
                             logAllThreads,
                             terminateProgram);
}
