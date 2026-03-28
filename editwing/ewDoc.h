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
//	Instances of classes derived from Command perform various operations
//	when invoked via operator(). Concrete implementations include
//	Insert, Delete, Replace, and MacroCommand.
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
//	Note: each Insert/Delete/Replace shifts string positions, but no automatic
//	position adjustment is performed. When composing sequences like
//	Insert->Delete->Insert, offsets must be calculated manually to account
//	for changed line and character counts. (^^;
//@}
//=========================================================================

class MacroCommand A_FINAL: public Command
{
public:
	//@{Add command //@}
	void Add( Command* cmd ) { if( cmd ) arr_.Add(cmd); } // do not save NULLs

	//@{Number of commands //@}
	ulong size() const { return arr_.size(); }

	//@{ destructor //@}
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
