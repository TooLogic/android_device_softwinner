/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

This directory tree contains the source code for third party packages used by the UpdateLogic
Device Agent.  Each package is briefly desribed below.  Please contact UpdateLogic, Inc. with
any questions regarding the use of these packages in the UpdateLogic Device Agent.


===============================================================================================
cJSON

This package is used by the live support functionality of the Device Agent and is only required
if PLATFORM_LIVE_SUPPORT is defined in the Makefile.  It provides the implementation of a JSON
parser.  JSON is best described at http://www.json.org/.

The license for this package does not preclude its free use and distribution.

See ThirdParty/cjson/33/README for additional information on this package.


===============================================================================================
gSOAP

This package is used by the live support functionality of the Device Agent and is only required
if PLATFORM_LIVE_SUPPORT is defined in the Makefile.  It provides the web services
infrastructure used by the Device Agent to communicate with the NetReady Services for live
support sessions.  gSOAP is best described at http://www.cs.fsu.edu/~engelen/soap.html.

UpdateLogic, Inc. has obtained a commercial license from Genivia, Inc. to support its use in
the UpdateLogic Device Agent, its distribution within the UpdateLogic Device Agent SDK and its
royalty free distribution in binary form within the UpdateLogic Device Agent on devices.

See ThirdParty/gsoap/2.7/README.txt for additional information on this package.


===============================================================================================
gstreamer

This package is used by the live support functionality of the Device Agent and is only required
if PLATFORM_LIVE_SUPPORT and PLATFORM_LIVE_SUPPORT_VIDEO are defined in the Makefile.  It
provides facilities for streaming device graphics out of the device for live support sessions.
See http://www.gstreamer.net for more background information on gstreamer.

The license for this package does not preclude linking to its libraries in a commercial
application.

See ThirdParty/gstreamer/README.txt for additional information on this package.

