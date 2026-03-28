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
//	A basic edit control that exposes character display, color changes, and cursor movement
//	by directly wrapping the core Doc/View functionality.
//	A split-window variant may be added in the future.
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
