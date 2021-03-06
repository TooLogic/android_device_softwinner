/* webservices_devicewebserviceClient.c
   Generated by gSOAP 2.7.15 from webservices_devicewebservice.h
   Copyright(C) 2000-2009, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#if defined(__BORLANDC__)
#pragma option push -w-8060
#pragma option push -w-8004
#endif
#include "webservices_devicewebserviceH.h"
#ifdef __cplusplus
extern "C" {
#endif

SOAP_SOURCE_STAMP("@(#) webservices_devicewebserviceClient.c ver 2.7.15 2010-11-18 13:44:23 GMT")


SOAP_FMAC5 int SOAP_FMAC6 soap_call___ns1__Ping1(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct _ns3__Ping1 *ns3__Ping1, struct _ns3__Ping1Response *ns3__Ping1Response)
{	struct __ns1__Ping1 soap_tmp___ns1__Ping1;
	if (!soap_endpoint)
		soap_endpoint = "http://chrisnigbur-l.updatelogic.lan:8080/2010/04/DeviceWebService.svc";
	if (!soap_action)
		soap_action = "http://uws.updatelogic.com/2010/04/DeviceWebService/IWebService/Ping1";
	soap->encodingStyle = NULL;
	soap_tmp___ns1__Ping1.ns3__Ping1 = ns3__Ping1;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns1__Ping1(soap, &soap_tmp___ns1__Ping1);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns1__Ping1(soap, &soap_tmp___ns1__Ping1, "-ns1:Ping1", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns1__Ping1(soap, &soap_tmp___ns1__Ping1, "-ns1:Ping1", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns3__Ping1Response)
		return soap_closesock(soap);
	soap_default__ns3__Ping1Response(soap, ns3__Ping1Response);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get__ns3__Ping1Response(soap, ns3__Ping1Response, "ns3:Ping1Response", "");
	if (soap->error)
	{	if (soap->error == SOAP_TAG_MISMATCH && soap->level == 2)
			return soap_recv_fault(soap);
		return soap_closesock(soap);
	}
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_call___ns1__GetWsProtocolVersion1(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct _ns3__GetWsProtocolVersion1 *ns3__GetWsProtocolVersion1, struct _ns3__GetWsProtocolVersion1Response *ns3__GetWsProtocolVersion1Response)
{	struct __ns1__GetWsProtocolVersion1 soap_tmp___ns1__GetWsProtocolVersion1;
	if (!soap_endpoint)
		soap_endpoint = "http://chrisnigbur-l.updatelogic.lan:8080/2010/04/DeviceWebService.svc";
	if (!soap_action)
		soap_action = "http://uws.updatelogic.com/2010/04/DeviceWebService/IWebService/GetWsProtocolVersion1";
	soap->encodingStyle = NULL;
	soap_tmp___ns1__GetWsProtocolVersion1.ns3__GetWsProtocolVersion1 = ns3__GetWsProtocolVersion1;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns1__GetWsProtocolVersion1(soap, &soap_tmp___ns1__GetWsProtocolVersion1);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns1__GetWsProtocolVersion1(soap, &soap_tmp___ns1__GetWsProtocolVersion1, "-ns1:GetWsProtocolVersion1", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns1__GetWsProtocolVersion1(soap, &soap_tmp___ns1__GetWsProtocolVersion1, "-ns1:GetWsProtocolVersion1", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns3__GetWsProtocolVersion1Response)
		return soap_closesock(soap);
	soap_default__ns3__GetWsProtocolVersion1Response(soap, ns3__GetWsProtocolVersion1Response);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get__ns3__GetWsProtocolVersion1Response(soap, ns3__GetWsProtocolVersion1Response, "ns3:GetWsProtocolVersion1Response", "");
	if (soap->error)
	{	if (soap->error == SOAP_TAG_MISMATCH && soap->level == 2)
			return soap_recv_fault(soap);
		return soap_closesock(soap);
	}
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_call___ns1__Hello1(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct _ns3__Hello1 *ns3__Hello1, struct _ns3__Hello1Response *ns3__Hello1Response)
{	struct __ns1__Hello1 soap_tmp___ns1__Hello1;
	if (!soap_endpoint)
		soap_endpoint = "http://chrisnigbur-l.updatelogic.lan:8080/2010/04/DeviceWebService.svc";
	if (!soap_action)
		soap_action = "http://uws.updatelogic.com/2010/04/DeviceWebService/IWebService/Hello1";
	soap->encodingStyle = NULL;
	soap_tmp___ns1__Hello1.ns3__Hello1 = ns3__Hello1;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns1__Hello1(soap, &soap_tmp___ns1__Hello1);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns1__Hello1(soap, &soap_tmp___ns1__Hello1, "-ns1:Hello1", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns1__Hello1(soap, &soap_tmp___ns1__Hello1, "-ns1:Hello1", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns3__Hello1Response)
		return soap_closesock(soap);
	soap_default__ns3__Hello1Response(soap, ns3__Hello1Response);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get__ns3__Hello1Response(soap, ns3__Hello1Response, "ns3:Hello1Response", "");
	if (soap->error)
	{	if (soap->error == SOAP_TAG_MISMATCH && soap->level == 2)
			return soap_recv_fault(soap);
		return soap_closesock(soap);
	}
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_call___ns1__Echo1(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct _ns3__Echo1 *ns3__Echo1, struct _ns3__Echo1Response *ns3__Echo1Response)
{	struct __ns1__Echo1 soap_tmp___ns1__Echo1;
	if (!soap_endpoint)
		soap_endpoint = "http://chrisnigbur-l.updatelogic.lan:8080/2010/04/DeviceWebService.svc";
	if (!soap_action)
		soap_action = "http://uws.updatelogic.com/2010/04/DeviceWebService/IWebService/Echo1";
	soap->encodingStyle = NULL;
	soap_tmp___ns1__Echo1.ns3__Echo1 = ns3__Echo1;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns1__Echo1(soap, &soap_tmp___ns1__Echo1);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns1__Echo1(soap, &soap_tmp___ns1__Echo1, "-ns1:Echo1", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns1__Echo1(soap, &soap_tmp___ns1__Echo1, "-ns1:Echo1", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns3__Echo1Response)
		return soap_closesock(soap);
	soap_default__ns3__Echo1Response(soap, ns3__Echo1Response);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get__ns3__Echo1Response(soap, ns3__Echo1Response, "ns3:Echo1Response", "");
	if (soap->error)
	{	if (soap->error == SOAP_TAG_MISMATCH && soap->level == 2)
			return soap_recv_fault(soap);
		return soap_closesock(soap);
	}
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_call___ns1__Control1(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct _ns3__Control1 *ns3__Control1, struct _ns3__Control1Response *ns3__Control1Response)
{	struct __ns1__Control1 soap_tmp___ns1__Control1;
	if (!soap_endpoint)
		soap_endpoint = "http://chrisnigbur-l.updatelogic.lan:8080/2010/04/DeviceWebService.svc";
	if (!soap_action)
		soap_action = "http://uws.updatelogic.com/2010/04/DeviceWebService/IWebService/Control1";
	soap->encodingStyle = NULL;
	soap_tmp___ns1__Control1.ns3__Control1 = ns3__Control1;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns1__Control1(soap, &soap_tmp___ns1__Control1);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns1__Control1(soap, &soap_tmp___ns1__Control1, "-ns1:Control1", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns1__Control1(soap, &soap_tmp___ns1__Control1, "-ns1:Control1", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns3__Control1Response)
		return soap_closesock(soap);
	soap_default__ns3__Control1Response(soap, ns3__Control1Response);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get__ns3__Control1Response(soap, ns3__Control1Response, "ns3:Control1Response", "");
	if (soap->error)
	{	if (soap->error == SOAP_TAG_MISMATCH && soap->level == 2)
			return soap_recv_fault(soap);
		return soap_closesock(soap);
	}
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

#ifdef __cplusplus
}
#endif

#if defined(__BORLANDC__)
#pragma option pop
#pragma option pop
#endif

/* End of webservices_devicewebserviceClient.c */
