#ifndef _TARBALLOMATIC_H_
#define _TARBALLOMATIC_H_


#include <Application.h>
#include <Catalog.h>
#include <Invoker.h>
#include <Locale.h>
#include <Message.h>


class TarballOMatic : public BApplication
{
public:
							TarballOMatic();
							~TarballOMatic();

	virtual	void			ReadyToRun();
	virtual	void			RefsReceived(BMessage* message);
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	
private:
			void			_SilentRelaunch();
			void			_UseExistingOrCreateNewWindow(BMessage*
								message = NULL);
			void			_StopArchiving();

			bool			fGotRefs;
			BInvoker*		fInvoker;
};

#endif // _TARBALLOMATIC_H_

