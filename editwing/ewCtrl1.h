#ifndef _EDITWING_CTRL1_H_
#define _EDITWING_CTRL1_H_
#include "ewDoc.h"
#include "ip_view.h"
#include "ip_doc.h"
#ifndef __ccdoc__
namespace editwing {
#endif



//=========================================================================
//@{ @pkg editwing.Ctrl //@}
//@{
//	easy edit controls
//
//	For now, the characters can be displayed, the colors can be changed, and the cursor can be moved...
//	This is a form that uses the basic functions of Doc/View as is.
//	I might make a split-window compatible version soon.
//@}
//=========================================================================

class EwEdit A_FINAL: public ki::WndImpl
{
public:

	EwEdit();
	~EwEdit();

public:

	//@{Document data manipulation //@}
	doc::Document& getDoc() { return doc_; }

	//@{ Display function operation //@}
	view::View& getView() { return *view_; }

	//@{ Cursor function operation //@}
	view::Cursor& getCursor() { return view_->cur(); }

private:

	doc::Document      doc_;
	view::View*        view_;
	static ClsName     className_;

private:

	void    on_create( CREATESTRUCT* cs ) override;
	void    on_destroy() override;
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp ) override;
};



//=========================================================================

}      // namespace editwing
#endif // _EDITWING_CTRL1_H_
