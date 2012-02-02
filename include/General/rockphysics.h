#ifndef rockphysics_h
#define rockphysics_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Feb 2012
 RCS:		$Id: rockphysics.h,v 1.2 2012-02-02 11:54:47 cvsbert Exp $
________________________________________________________________________


-*/

#include "propertyref.h"
#include "repos.h"
class MathProperty;
class ascistream;
class ascostream;


/*!\brief Ref Data for a (usually petrophysical) property.

We prepare for many variants of the name as is not uncommon in practice
(Density, Den, Rho, RhoB, ... you know the drill). The names will be unique
- case insensitive, in the Set. Hence, identity is established case insensitive.
Aliases are matched with a GlobExpr, so you can add with wildcards and the like.

 */


namespace RockPhysics
{

mClass Formula : public NamedObject
{
public:

    typedef PropertyRef::StdType OutputType;

			Formula( OutputType t, const char* nm=0 )
			: NamedObject(nm)
			, type_(t)		{}
    static Formula*	get(const IOPar&);	//!< returns null if bad IOPar
			~Formula()		{ deepErase(constdefs_); }
			Formula( const Formula& f ) { *this = f; }
    Formula&		operator =(const Formula&);
    inline bool		operator ==( const Formula& pr ) const
			{ return name() == pr.name(); }
    inline bool		operator !=( const Formula& pr ) const
			{ return name() != pr.name(); }

    inline bool		hasOutputType( OutputType t ) const
						{ return type_ == t; }

    mClass ConstDef : public NamedObject
    {
    public:
			ConstDef( const char* nm )
			    : NamedObject(nm)
			    , typicalrg_(0,1)		{}
	BufferString	desc_;
	Interval<float>	typicalrg_;

    };

    OutputType		type_;
    BufferString	def_;
    BufferString	desc_;
    ObjectSet<ConstDef>	constdefs_;
    BufferStringSet	vardefs_;
    Repos::Source	src_;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    bool		setDef(const char*); // Will add var- and constdefs
    MathProperty*	getProperty(const PropertyRef* pr=0) const;

};


mClass FormulaSet : public ObjectSet<const Formula>
{
public:
    			~FormulaSet()
			{ deepErase( *(ObjectSet<Formula>*)this ); }
    int			getIndexOf(const char*) const;
    void		getRelevant(PropertyRef::StdType,
	    			    BufferStringSet&) const;

    const Formula*	get( const char* nm ) const
			{
			    const int idxof = getIndexOf( nm );
			    return idxof<0 ? 0 : (*this)[idxof];
			}

    bool		save(Repos::Source) const;

    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;

};



} // namespace RockPhysics

const RockPhysics::FormulaSet& ROCKPHYSFORMS();
inline RockPhysics::FormulaSet& eROCKPHYSFORMS()
{ return const_cast<RockPhysics::FormulaSet&>( ROCKPHYSFORMS() ); }


#endif
