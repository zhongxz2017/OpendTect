#ifndef uiwindowfunctionsel_h
#define uiwindowfunctionsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.h,v 1.2 2007-10-18 10:25:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class WindowFunction;
class uiGenInput;
class uiWindowFuncSelDlg;

/*!Selects a windowfunction and its eventual parameter. */

class uiWindowFunctionSel : public uiGroup
{
public:
    			uiWindowFunctionSel(uiParent*,const char* label,
					    const char* curwinname = 0,
					    float curwinparam = mUdf(float) );
    			~uiWindowFunctionSel();

    NotifierAccess&	typeChange();

    void		setWindowName(const char*);
    void		setWindowParamValue(float);

    const char*		windowName() const;
    float		windowParamValue() const;
    const char*		windowParamName() const;

    static const char*	sNone() { return "None"; }

protected:
    void			windowChangedCB(CallBacker*);
    void			winfuncseldlgCB(CallBacker*);

    BufferString		errmsg_;
    Interval<float>		annotrange_;

    uiGenInput*			windowtypefld_;
    uiWindowFuncSelDlg*		winfuncseldlg_;
    ObjectSet<uiGenInput>	windowvariable_;
    ObjectSet<WindowFunction>	windowfuncs_;
};


#endif
