//hDibImport.h

#ifndef __hDibImport_h_
#define __hDibImport_h_

#include "HColor.h"

class CDibImport
{
private:
	void* m_Import;
public:
    CDibImport():m_Import(0) {}
    void Inti(const long width,const long height);
    void Clear();
    ~CDibImport() { Clear(); }
	void*   lock_DC();
	void    unlock_DC();
	bool    lock_Data(Context32& out_dst);
	void    unlock_Data(Context32& dst);
};

#endif //__hDibImport_h_
