#ifndef seisread_h
#define seisread_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seisread.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

A SeisTrcReader reads from a seismic data store. If you don't want all of the data,
you must set the KeyData after construction. To be able to use the reader, you
need to execute the starter() executable. Then you may find out the storage
characteristics of the traces from the translator or the SeisPacketInfo.

Then, the routine is: get(trc.info()) possibly followed by get(trc). Not keeping
this sequence is at your own risk.

@$*/

#include <seisstor.h>
class StorageLayout;
class SeisKeyData;


class SeisTrcReader : public SeisStorage
{

    friend class	SeisReadStarter;

public:

			SeisTrcReader(const IOObj* =0);
    Executor*		starter();

    const StorageLayout& storageLayout() const;
    void		setKeyData(const SeisKeyData&);
    const SeisKeyData&	keyData() const;

    int			get(SeisTrcInfo&);
			    // -1 = Error. errMsg() will return a message.
			    //  0 = End
			    //  1 = Usable info
			    //  2 = Not usable (skipped the trace)
			    // If get(SeisTrc) is not called, get(SeisTrcInfo)
			    // will automatically skip over the trace data
			    // if necessary.
			
    bool		get(SeisTrc&);
			    // You should call this function only if
			    // get(trc.info()) returned 1. If you don't,
			    // the trace selections may be ignored.

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:

    int			itrc;
    bool		icfound;
    bool		new_packet;
    bool		needskip;

    void		init();
    bool		openFirst();
    bool		initRead();
    void		trySkipFiles(bool);
    int			nextConn(SeisTrcInfo&);
    void		finishGetInfo(SeisTrcInfo&);

};


#endif
