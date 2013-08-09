/* utv-cem.h: UpdateTV CEM specific defines

 Copyright (c) 2010 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      02/15/2010  Created.
*/

#ifndef __UTV_CEM_H__
#define __UTV_CEM_H__

/* The 24-bit IEEE Organizational Unique Identifier for this CEM.
*/
#define UTV_CEM_OUI                    0xC09C92

/* The Model Group defined for RELEASE versions - this can otherwise
   be installed/built into the manifest for DEBUG versions
*/
#define UTV_CEM_MODEL_GROUP            0x0001

/* These hardware model values must have counterparts on the NOC.
   They represent a virualization of the CEM-specific hardware model.

   Hardware models are typically panel-size/panel-manufacturer related.
   They exist to allow an update to be targeted at a particular hardware
   model if it has a problem so that other hardware models without the problem
   aren't updated.  This is done because updates can disturb the consumer. 
   
*/
#define UTV_CEM_HW_MODEL_1             0x0001
#define UTV_CEM_HW_MODEL_2             0x0002
#define UTV_CEM_HW_MODEL_3             0x0003

#endif /* __UTV_CEM_H__ */
