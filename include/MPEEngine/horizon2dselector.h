#ifndef horizon2dselector_h
#define horizon2dselector_h

/*+
________________________________________________________________________
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id$
________________________________________________________________________
*/

#include "mpeenginemod.h"
#include "sectionselector.h"

namespace EM { class Horizon2D; }

namespace MPE
{

/*!
\brief SectionSourceSelector to select EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DSelector : public SectionSourceSelector
{
public:
    				Horizon2DSelector(const EM::Horizon2D&,
						  const EM::SectionID&);
    int				nextStep();

protected:
    const EM::Horizon2D&	horizon_;
};

} // namespace MPE

#endif

