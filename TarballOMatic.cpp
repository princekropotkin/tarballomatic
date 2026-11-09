/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */

#include "TarballOMatic.h"

#include <Alert.h>
#include <Roster.h>
#include <StringFormat.h>
#include <private/shared/TrackerAddOnAppLaunch.h>

#include "TarballOMaticMisc.h"
#include "TarballOMaticWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "file:TarballOMatic.cpp"


int
main()
{
	TarballOMatic app;
	app.Run();

	return 0;
}


TarballOMatic::TarballOMatic()
	:
	BApplication(TARBALLOMATIC_APP_SIG),
	fGotRefs(false),
	fInvoker(new BInvoker(new BMessage(TARBALL_QUIT_OR_CONTINUE), NULL, this))
{
}


TarballOMatic::~TarballOMatic()
{
}


void
TarballOMatic::RefsReceived(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK) {
		_UseExistingOrCreateNewWindow(message);
		fGotRefs = true;
	} else if (!IsLaunching()) {
		PostMessage(B_SILENT_RELAUNCH);
	}
}


void
TarballOMatic::ReadyToRun()
{
	if (!fGotRefs)
		_UseExistingOrCreateNewWindow();
}


void
TarballOMatic::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case TARBALL_WINDOW_QUIT:
		{
			snooze(200000);
			if (CountWindows() == 0)
				Quit();
			break;
		}
		case B_SILENT_RELAUNCH:
			_SilentRelaunch();
			break;

		case TARBALL_QUIT_OR_CONTINUE:
		{
			int32 button;
			if (message->FindInt32("which", &button) == B_OK) {
				if (button == 0) {
					_StopArchiving();
				} else {
					if (CountWindows() == 0)
						Quit();
				}
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool
TarballOMatic::QuitRequested(void)
{
	if (CountWindows() <= 0)
		return true;

	BWindow* window;
	TarballWindow* tarball;
	TarballWindow* lastFoundTarball = NULL;
	int32 tarballCount = 0;

	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;

		tarball = dynamic_cast<TarballWindow*>(window);
		if (tarball == NULL)
			continue;

		lastFoundTarball = tarball;

		if (tarball->Lock()) {
			if (tarball->IsArchiving())
				tarballCount++;
			else
				tarball->PostMessage(B_QUIT_REQUESTED);

			tarball->Unlock();
		}
	}

	if (tarballCount == 1) {
		// This is likely the most frequent case - a single archiver.
		// We post a message to the window so it can put up its own
		// BAlert instead of the app-wide BAlert. This avoids making
		// a difference between having pressed Commmand-W or Command-Q.
		// Closing or quitting, it doesn't matter for a single window.

		if (lastFoundTarball->Lock()) {
			lastFoundTarball->Activate();
			lastFoundTarball->PostMessage(B_QUIT_REQUESTED);
			lastFoundTarball->Unlock();
		}
		return false;
	}

	if (tarballCount > 0) {
		// The multi-archiver case differs from the single-archiver case
		// in that archivers are not paused while the BAlert is up.

		static BStringFormat format(
			B_TRANSLATE("You have {0, plural, one{# Tarball-O-Matic} "
				"other{# Tarball-O-Matics}} running.\n\n"));

		BString question;

		format.Format(question, tarballCount);

		question << B_TRANSLATE("Do you want to stop them?");

		BAlert* alert = new BAlert(NULL, question.String(),
			B_TRANSLATE("Stop them"), B_TRANSLATE("Let them continue"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(fInvoker);
		alert->Activate();
			// BAlert, being modal, does not show on the current workspace
			// if the application has no window there. Activate() triggers
			// a switch to a workspace where it does have a window.

			// TODO: See if AS_ACTIVATE_WINDOW should be handled differently
			// in src/servers/app/Desktop.cpp Desktop::ActivateWindow()
			// or if maybe BAlert should (and does not?) activate itself.

		return false;
	}

	if (CountWindows() <= 0)
		return true;

	return false;
}


void
TarballOMatic::_SilentRelaunch()
{
	_UseExistingOrCreateNewWindow();
}


void
TarballOMatic::_UseExistingOrCreateNewWindow(BMessage* message)
{
	int32 windowCount = 0;
	BWindow* bWindow;
	TarballWindow* zWindow;
	BList list;

	while (1) {
		bWindow = WindowAt(windowCount++);
		if (bWindow == NULL)
			break;

		zWindow = dynamic_cast<TarballWindow*>(bWindow);
		if (zWindow == NULL)
			continue;

		list.AddItem(zWindow);

		if (zWindow->Lock()) {
			if (!zWindow->IsArchiving()) {
				if (message != NULL)
					zWindow->PostMessage(message);
				zWindow->SetWorkspaces(B_CURRENT_WORKSPACE);
				zWindow->Activate();
				zWindow->Unlock();
				return;
			}
			zWindow->Unlock();
		}
	}

	if (message) {
		zWindow = new TarballWindow(list);
		zWindow->PostMessage(message);
	} else {
		zWindow = new TarballWindow(list, true);
	}

	zWindow->Show();
}


void
TarballOMatic::_StopArchiving()
{
	BWindow* window;
	TarballWindow* tarball;
	BList list;

	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;

		tarball = dynamic_cast<TarballWindow*>(window);
		if (tarball == NULL)
			continue;

		list.AddItem(tarball);
	}

	for (int32 i = 0;; i++) {
		tarball = static_cast<TarballWindow*>(list.ItemAt(i));
		if (tarball == NULL)
			break;

		if (tarball->Lock()) {
			if (tarball->IsArchiving())
				tarball->StopArchiving();

			tarball->PostMessage(B_QUIT_REQUESTED);
			tarball->Unlock();
		}
	}
}

