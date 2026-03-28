#ifndef _EDITWING_DOC_H_
#define _EDITWING_DOC_H_
#include "ewCommon.h"
#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



class Document;
class DocEvHandler;
class Command;
class Insert;
class Delete;
class Replace;


//=========================================================================
//@{
//	event handler interface
//
//	Events that occur from the document (insert/delete, etc.)
//	If you want to receive it, inherit this interface and set the handler accordingly.
//	writing. View redrawing processing is also executed through this.
//@}
//=========================================================================

class A_NOVTABLE DocEvHandler
{
public:
	//@{
	//	Occurs when the text content changes
	//	@param s Start of change range
	//	@param e End of change range (previous)
	//	@param e2 End of change range (after)
	//	@param reparsed True if the commented out state after e2 has changed
	//	@param nmlcmd true for insert/delete/replace, false for open file/replace all
	//@}
	virtual void on_text_update( const DPos& s,
		const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd ) {}

	//@{
	//	Occurs when a keyword is changed
	//@}
	virtual void on_keyword_change() {}

	//@{
	//	Occurs when the dirty flag is changed
	//@}
	virtual void on_dirtyflag_change( bool dirty ) {}
};



//=========================================================================
//@{
//	Operation command interface
//
//	The documentation for instances of classes derived from Command
//	Perform various operations by calling operator(). for now
//	Specifically, there are only three: Insert/Delete/Replace. later for macro commands
//	I'm planning to create a class, but I'll put it on hold for now.
//@}
//=========================================================================

class A_NOVTABLE Command : public ki::Object
{
protected:
	friend class UnReDoChain;
	friend class MacroCommand;
	virtual Command* operator()( Document& doc ) const = 0;
public:
	virtual ~Command() {};
};



//=========================================================================
//@{
//	insert command
//@}
//=========================================================================

class Insert A_FINAL: public Command
{
public:

	//@{
	//	@param s insertion position
	//	@param str insert string
	//	@param len String length
	//	@param del Can I delete [] str when the command ends?
	//@}
	Insert( const DPos& s, const unicode* str, ulong len, bool del=false );
	~Insert() override;

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	const unicode* buf_;
	ulong          len_ : sizeof(ulong)*8-1;
	ulong          del_ : 1;
};



//=========================================================================
//@{
//	Delete command
//@}
//=========================================================================

class Delete A_FINAL: public Command
{
public:

	//@{
	//	@param s start position
	//	@param e end position
	//@}
	Delete( const DPos& s, const DPos& e );

private:

	Command* operator()( Document& doc ) const override;
	DPos stt_;
	DPos end_;
};




//=========================================================================
//@{
//	Replacement command
//@}
//=========================================================================

class Replace A_FINAL: public Command
{
public:

	//@{
	//	@param s start position
	//	@param e end position
	//	@param str insert string
	//	@param len String length
	//	@param del Can I delete [] str when the command ends?
	//@}
	Replace( const DPos& s, const DPos& e,
		const unicode* str, ulong len, bool del=false );
	~Replace() override;

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	DPos           end_;
	const unicode* buf_;
	ulong          len_ : sizeof(ulong)*8-1;
	ulong          del_ : 1;
};



//=========================================================================
//@{
//	macro command
//
//	Continuously execute multiple commands as one command.
//	However, each time you perform Insert/Delete/Replace, the
//	The position of the string changes, but the conversion process is
//	Don't do it. i.e. something like Insert->Delete->Insert
//	When writing continuous processing, consider changes in the number of lines and characters.
//	It is necessary to determine the value. So I can't use it much (^^;
//@}
//=========================================================================

class MacroCommand A_FINAL: public Command
{
public:
	//@{Add command //@}
	void Add( Command* cmd ) { if( cmd ) arr_.Add(cmd); } // do not save NULLs

	//@{Number of commands //@}
	ulong size() const { return arr_.size(); }

	//@ destructor //@}
	~MacroCommand() override
	{
		for( ulong i=0,e=arr_.size(); i<e; ++i )
			delete arr_[i];
	}

	MacroCommand() : arr_(16) {}

private:
	Command* operator()( Document& doc ) const override;
	ki::storage<Command*> arr_;
};



//=========================================================================

}}     // namespace editwing::document
#endif // _EDITWING_DOC_H_
