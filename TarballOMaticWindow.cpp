/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Copyright 2026, Prince Kropotkin. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 *		Prince Kropotkin, haiku@kropotk.in
 */


#include "TarballOMaticWindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringFormat.h>

#include "TarballOMatic.h"
#include "TarballOMaticActivity.h"
#include "TarballOMaticMisc.h"
#include "TarballThread.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "file:TarballOMaticWindow.cpp"


TarballWindow::TarballWindow(BList windowList, bool keepOpen)
	:
	BWindow(BRect(0, 0, 0, 0), "Tarball-O-Matic", B_TITLED_WINDOW,
		B_NOT_RESIZABLE	| B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
	fWindowList(windowList),
	fThread(NULL),
	fKeepOpen(keepOpen),
	fArchivingWasStopped(false),
	fFileCount(0),
	fWindowInvoker(new BInvoker(new BMessage(TARBALL_QUIT_OR_CONTINUE), NULL,
		this))
{
	fActivityView = new Activity("activity");
	fActivityView->SetExplicitMinSize(BSize(171, 17));

	fArchiveNameView = new BStringView("archive_text", "");
	fArchiveNameView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	fTarOutputView = new BStringView("output_text",
		B_TRANSLATE("Drop files here."));
	fTarOutputView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	fStopButton = new BButton("stop", B_TRANSLATE("Stop"),
		new BMessage(B_QUIT_REQUESTED));
	fStopButton->SetEnabled(false);
	fStopButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
		B_ALIGN_VERTICAL_UNSET));

	BSeparatorView* separator = new BSeparatorView(B_HORIZONTAL);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 10)
			.Add(fActivityView)
			.Add(fArchiveNameView)
			.Add(fTarOutputView)
			.Add(separator)
			.Add(fStopButton)
			.SetInsets(14, 14, 14, 14)
			.End()
		.End();

	_FindBestPlacement();
}


TarballWindow::~TarballWindow()
{
	delete fWindowInvoker;
}


void
TarballWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			if (IsArchiving()) {
				message->what = B_REFS_RECEIVED;
				be_app_messenger.SendMessage(message);
			} else {
				_StartArchiving(message);
			}
			break;

		case TARBALL_THREAD_EXIT:
			fThread = NULL;
			fActivityView->Stop();
			fStopButton->SetEnabled(false);
			fArchiveNameView->SetText(" ");
			if (fArchivingWasStopped)
				fTarOutputView->SetText(B_TRANSLATE("Stopped"));
			else
				fTarOutputView->SetText(B_TRANSLATE("Archive created OK"));

			_CloseWindowOrKeepOpen();
			break;

		case TARBALL_THREAD_EXIT_ERROR:
			// TODO: figure out why this case does not happen when it should
			fThread = NULL;
			fActivityView->Stop();
			fStopButton->SetEnabled(false);
			fArchiveNameView->SetText("");
			fTarOutputView->SetText(B_TRANSLATE("Error creating archive"));
			break;

		case TARBALL_TASK_DESCRIPTION:
		{
			BString filename;
			if (message->FindString("archive_filename", &filename) == B_OK) {
				fArchiveName = filename;
				BString temp(B_TRANSLATE("Creating archive: %s"));
				temp.ReplaceFirst("%s", filename.String());
				fArchiveNameView->SetText(temp.String());
			}
			break;
		}

		case TARBALL_LINE_OF_STDOUT:
		{
			BString string;
			if (message->FindString("tar_output", &string) == B_OK) {
				if (string.FindFirst("Adding: ") == 0) {

					// This is a workaround for the current window resizing
					// behavior as the window resizes for each line of output.
					// Instead of showing the true output of bsdtar
					// we display a file count and whether the archive is
					// being created (added to) or if we're updating an
					// already existing archive.

					static BStringFormat format(B_TRANSLATE("{0, plural, "
						"one{# file added.} other{# files added.}}"));
					BString output;
					fFileCount++;

					format.Format(output, fFileCount);

					fTarOutputView->SetText(output.String());
				} else {
					fTarOutputView->SetText(string.String());
				}
			}
			break;
		}

		case TARBALL_QUIT_OR_CONTINUE:
		{
			int32 which_button = -1;
			if (message->FindInt32("which", &which_button) == B_OK) {
				if (which_button == 0) {
					StopArchiving();
				} else {
					if (fThread != NULL)
						fThread->ResumeExternalTar();

					fActivityView->Start();
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TarballWindow::QuitRequested()
{
	if (!IsArchiving()) {
		BMessage message(TARBALL_WINDOW_QUIT);
		message.AddRect("frame", Frame());
		be_app_messenger.SendMessage(&message);
		return true;
	} else {
		fThread->SuspendExternalTar();
		fActivityView->Pause();

		BString message;
		message << B_TRANSLATE("Are you sure you want to stop creating this "
			"archive?");
		message << "\n\n";
		message << B_TRANSLATE("Filename: %s");
		message << "\n";
		message.ReplaceFirst("%s", fArchiveName.String());

		BAlert* alert = new BAlert(NULL, message.String(), B_TRANSLATE("Stop"),
			B_TRANSLATE("Continue"), NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(fWindowInvoker);

		return false;
	}
}


void
TarballWindow::_StartArchiving(BMessage* message)
{
	fStopButton->SetEnabled(true);
	fActivityView->Start();
	fFileCount = 0;

	fThread = new TarballThread(message, this);
	fThread->Start();

	fArchivingWasStopped = false;
}


void
TarballWindow::StopArchiving()
{
	fArchivingWasStopped = true;

	fStopButton->SetEnabled(false);
	fActivityView->Stop();

	fThread->InterruptExternalTar();
	fThread->Quit();

	status_t status = B_OK;
	fThread->WaitForThread(&status);
	fThread = NULL;

	fArchiveNameView->SetText(" ");
	fTarOutputView->SetText(B_TRANSLATE("Stopped"));

	_CloseWindowOrKeepOpen();
}


bool
TarballWindow::IsArchiving()
{
	if (fThread == NULL)
		return false;
	else
		return true;
}


void
TarballWindow::_CloseWindowOrKeepOpen()
{
	if (!fKeepOpen)
		PostMessage(B_QUIT_REQUESTED);
}


void
TarballWindow::_FindBestPlacement()
{
	CenterOnScreen();

	BScreen screen;
	BRect centeredRect = Frame();
	BRect tryRect = centeredRect;
	BList tryRectList;

	if (!screen.Frame().Contains(centeredRect))
		return;

	// build a list of possible locations
	tryRectList.AddItem(new BRect(centeredRect));

	// up and left
	direction primaryDirection = up;
	while (true) {
		_OffsetRect(&tryRect, primaryDirection);

		if (!screen.Frame().Contains(tryRect))
			_OffscreenBounceBack(&tryRect, &primaryDirection, left);

		if (!screen.Frame().Contains(tryRect))
			break;

		tryRectList.AddItem(new BRect(tryRect));
	}

	// down and right
	primaryDirection = down;
	tryRect = centeredRect;
	while (true) {
		_OffsetRect(&tryRect, primaryDirection);

		if (!screen.Frame().Contains(tryRect))
			_OffscreenBounceBack(&tryRect, &primaryDirection, right);

		if (!screen.Frame().Contains(tryRect))
			break;

		tryRectList.AddItem(new BRect(tryRect));
	}

	// remove rects that overlap an existing window
	for (int32 i = 0;; i++) {
		BWindow* win = static_cast<BWindow*>(fWindowList.ItemAt(i));
		if (win == NULL)
			break;

		TarballWindow* window = dynamic_cast<TarballWindow*>(win);
		if (window == NULL)
			continue;

		if (window == this)
			continue;

		if (window->Lock()) {
			BRect frame = window->Frame();
			for (int32 m = 0;; m++) {
				BRect* rect = static_cast<BRect*>(tryRectList.ItemAt(m));
				if (rect == NULL)
					break;

				if (frame.Intersects(*rect)) {
					tryRectList.RemoveItem(m);
					delete rect;
					m--;
				}
			}
			window->Unlock();
		}
	}

	// find nearest rect
	bool gotRect = false;
	BRect nearestRect(0, 0, 0, 0);

	while (true) {
		BRect* rect = static_cast<BRect*>(tryRectList.RemoveItem((int32)0));
		if (rect == NULL)
			break;

		nearestRect = _NearestRect(centeredRect, nearestRect, *rect);
		gotRect = true;
		delete rect;
	}

	if (gotRect)
		MoveTo(nearestRect.LeftTop());
}


void
TarballWindow::_OffsetRect(BRect* rect, direction whereTo)
{
	float width = rect->Width();
	float height = rect->Height();

	switch (whereTo) {
		case up:
			rect->OffsetBy(0, -(height * 1.5));
			break;

		case down:
			rect->OffsetBy(0, height * 1.5);
			break;

		case left:
			rect->OffsetBy(-(width * 1.5), 0);
			break;

		case right:
			rect->OffsetBy(width * 1.5, 0);
			break;
	}
}


void
TarballWindow::_OffscreenBounceBack(BRect* rect, direction* primaryDirection,
	direction secondaryDirection)
{
	if (*primaryDirection == up) {
		*primaryDirection = down;
	} else {
		*primaryDirection = up;
	}

	_OffsetRect(rect, *primaryDirection);
	_OffsetRect(rect, secondaryDirection);
}


BRect
TarballWindow::_NearestRect(BRect goalRect, BRect a, BRect b)
{
	double aSum = fabs(goalRect.left - a.left) + fabs(goalRect.top - a.top);
	double bSum = fabs(goalRect.left - b.left) + fabs(goalRect.top - b.top);

	if (aSum < bSum)
		return a;
	else
		return b;
}

