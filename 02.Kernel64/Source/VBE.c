#include "VBE.h"

static VBEMODEINFOBLOCK *VBEModeBlockInfo = (VBEMODEINFOBLOCK*)VBE_MODEINFOBLOCKADDRESS;

inline VBEMODEINFOBLOCK *GetVBEInfoBlock(void) {
	return VBEModeBlockInfo;
}
