#ifndef KVIM_IFACE
#define KVIM_IFACE

#include <dcopobject.h>

class KVim : virtual public DCOPObject
{
	K_DCOP
public:

k_dcop:
	virtual void execInsert(QString command)=0;
	virtual void execNormal(QString command)=0;
	virtual void execRaw(QString command)=0;
	virtual void execCmd(QString command)=0;
	virtual QString eval(QString expr)=0;
};

#endif
