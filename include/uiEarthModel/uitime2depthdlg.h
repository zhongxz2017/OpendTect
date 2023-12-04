#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

#include "emioobjinfo.h"
#include "emsurface.h"
#include "zaxistransform.h"

class IOObjContext;
class uiGenInput;
class uiSurfaceRead;
class uiSurfaceWrite;
class uiZAxisTransformSel ;
class uiLabel;
namespace ZDomain { class Info; }

namespace EM
{
mExpClass(uiEarthModel) uiTime2DepthDlg  : public uiDialog
{ mODTextTranslationClass(uiEMObjectTime2DepthDlg )
public:
				uiTime2DepthDlg (uiParent*,
					    IOObjInfo::ObjectType);
				~uiTime2DepthDlg ();

    static uiRetVal		canTransform(IOObjInfo::ObjectType);

protected:

    uiGenInput*			directionsel_		= nullptr;
    uiZAxisTransformSel*	t2dtransfld_		= nullptr;
    uiZAxisTransformSel*	d2ttransfld_		= nullptr;
    uiSurfaceRead*		inptimehorsel_		= nullptr;
    uiSurfaceRead*		inpdepthhorsel_		= nullptr;
    uiSurfaceWrite*		outtimehorsel_		= nullptr;
    uiSurfaceWrite*		outdepthhorsel_		= nullptr;

    uiString			getDlgTitle(IOObjInfo::ObjectType) const;

    const ZDomain::Info&	outZDomain() const;

    RefMan<ZAxisTransform>	getWorkingZAxisTransform() const;
    const uiSurfaceRead*	getWorkingInpSurfRead() const;
    uiSurfaceWrite*		getWorkingOutSurfWrite();

    void			dirChangeCB(CallBacker*);
    void			horSelCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    bool			is2DObject() const;

    const IOObjInfo::ObjectType objtype_;
};
}