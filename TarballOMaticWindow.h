#ifndef _TARBALLOMATIC_WINDOW_H
#define _TARBALLOMATIC_WINDOW_H


#include <Button.h>
#include <List.h>
#include <Rect.h>
#include <StringView.h>
#include <Window.h>

#include "TarballOMaticActivity.h"
#include "TarballThread.h"


enum direction {
	up,
	down,
	left,
	right
};


class TarballWindow : public BWindow
{
public:
							TarballWindow(BList windowList,
								bool keepOpen = false);
							~TarballWindow();
							
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	
			bool			IsArchiving();
			void			StopArchiving();
			
private:

			void			_StartArchiving(BMessage* message);
			void			_CloseWindowOrKeepOpen();

			void			_FindBestPlacement();
			void			_OffsetRect(BRect* rect, direction whereTo);
			void			_OffscreenBounceBack(BRect* rect,
								direction* primaryDirection,
								direction secondaryDirection);
			BRect			_NearestRect(BRect goalRect, BRect a, BRect b);

			BList			fWindowList;

			Activity*		fActivityView;
			BStringView*	fArchiveNameView;
			BStringView*	fTarOutputView;
			BButton*		fStopButton;

			TarballThread*	fThread;
			BString			fArchiveName;
	
			bool			fKeepOpen;
			bool			fArchivingWasStopped;
			int32			fFileCount;
			
			BInvoker*		fWindowInvoker;
};

#endif	// _TARBALLOMATIC_WINDOW_H

