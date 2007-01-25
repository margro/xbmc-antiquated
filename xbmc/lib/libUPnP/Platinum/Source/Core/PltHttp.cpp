/*****************************************************************
|
|   Platinum - HTTP Protocol Helper
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptThreads.h"
#include "NptUtils.h"
#include "NptXml.h"
#include "NptVersion.h"
#include "PltHttp.h"
#include "PltLog.h"
#include "PltDatagramStream.h"
#include "PltVersion.h"

static NPT_Mutex Lock;

/*----------------------------------------------------------------------
|   NPT_HttpHeaderFinder
+---------------------------------------------------------------------*/
class NPT_HttpHeaderFinder
{
 public:
    // methods
    NPT_HttpHeaderFinder(const char* name) : m_Name(name) {}
    bool operator()(const NPT_HttpHeader* const & header) const {
		if (header->GetName().Compare(m_Name, true)) {
			return true;
		} else {
			return false;
		}
    }

 private:
    // members
    NPT_String m_Name;
};

/*----------------------------------------------------------------------
|   NPT_HttpHeaderPrinter
+---------------------------------------------------------------------*/
class NPT_HttpHeaderPrinter
{
public:
    // methods
    NPT_HttpHeaderPrinter(NPT_OutputStreamReference& stream) : m_Stream(stream) {}
    NPT_Result operator()(NPT_HttpHeader*& header) const {
        m_Stream->WriteString(header->GetName());
        m_Stream->Write(": ", 2);
        m_Stream->WriteString(header->GetValue());
        m_Stream->Write("\r\n", 2, NULL);
        return NPT_SUCCESS;
    }

private:
    // members
    NPT_OutputStreamReference& m_Stream;
};

/*----------------------------------------------------------------------
|   NPT_HttpHeaderLogger
+---------------------------------------------------------------------*/
class NPT_HttpHeaderLogger
{
public:
    // methods
    NPT_HttpHeaderLogger(unsigned long level = PLT_LOG_LEVEL_3) : m_level(level) {}
	NPT_Result operator()(NPT_HttpHeader*& header) const {
        PLT_Log(m_level, "%s: %s\r\n", (const char*)header->GetName(), (const char*)header->GetValue());
        return NPT_SUCCESS;
    }

    unsigned long m_level;
};


/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetContentType
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetContentType(NPT_HttpMessage* message, NPT_String& type) 
{ 
    return message->GetHeaders().GetHeaderValue("Content-Type", type);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetContentType
+---------------------------------------------------------------------*/
void
PLT_HttpHelper::SetContentType(NPT_HttpMessage* message, const char* type)   
{
    message->GetHeaders().SetHeader("Content-Type", type);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetContentLength
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetContentLength(NPT_HttpMessage* message, NPT_Size& len) 
{ 
    long out;
    NPT_String value;
    if (NPT_FAILED(message->GetHeaders().GetHeaderValue("Content-Length", value))) {
        return NPT_FAILURE;
    }

    NPT_Result res = value.ToInteger(out);
    len = out;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetContentLength
+---------------------------------------------------------------------*/
void
PLT_HttpHelper::SetContentLength(NPT_HttpMessage* message, NPT_Size len)   
{
    message->GetHeaders().SetHeader("Content-Length", NPT_String::FromInteger(len));
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetBody(NPT_HttpMessage* message, NPT_String& body)
{
    return SetBody(message, (const char*)body, body.GetLength());
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::SetBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetBody(NPT_HttpMessage* message, const char* body, NPT_Size len)
{
    if (len == 0) {
        return NPT_SUCCESS;
    }

    // dump the body in a memory stream
    NPT_MemoryStreamReference stream(new NPT_MemoryStream);
    stream->Write(body, len);

    // set content length
    PLT_HttpHelper::SetContentLength(message, len);

    return SetBody(message, (NPT_InputStreamReference&)stream, len);
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::SetBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetBody(NPT_HttpMessage* message, NPT_InputStreamReference& stream, NPT_Size len)
{
    if (len == 0) {
        NPT_CHECK(stream->GetAvailable(len));
    }

    // get the entity
    NPT_HttpEntity* entity = message->GetEntity();
    if (entity == NULL) {
        // no entity yet, create one
        message->SetEntity(entity = new NPT_HttpEntity());
    }

    // set the entity body
    entity->SetInputStream(stream);
    entity->SetContentLength(len);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetBody
+---------------------------------------------------------------------*/
NPT_Result 
PLT_HttpHelper::GetBody(NPT_HttpMessage& message, NPT_String& body) 
{
    NPT_Result res;
    NPT_InputStreamReference stream;

    // get stream
    NPT_HttpEntity* entity = message.GetEntity();
    if (!entity || NPT_FAILED(entity->GetInputStream(stream))) {
        return NPT_FAILURE;
    }

    // extract body
    NPT_StringOutputStream* output_stream = new NPT_StringOutputStream(&body);
    res = NPT_StreamToStreamCopy(*stream, *output_stream, 0, entity->GetContentLength());
    delete output_stream;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::ParseBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ParseBody(NPT_HttpMessage& message, NPT_XmlElementNode*& tree) 
{
    // reset tree
    tree = NULL;

    // read body
    NPT_String body;
    NPT_CHECK(GetBody(message, body));

    // parse body
    NPT_XmlParser parser;
    NPT_XmlNode*  node;
    NPT_CHECK(parser.Parse(body, node));
    
    tree = node->AsElementNode();
    if (!tree) {
        delete node;
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ToLog(NPT_HttpMessage* message, unsigned long level)
{
    //NPT_AutoLock lock(Lock);
    message->GetHeaders().GetHeaders().Apply(NPT_HttpHeaderLogger(level));
    PLT_Log(level, "\r\n");
    //if (!m_Body.IsNull()) {
    //    m_Body->Log(level);
    //}
    PLT_Log(level, "\r\n");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::IsConnectionKeepAlive
+---------------------------------------------------------------------*/
bool
PLT_HttpHelper::IsConnectionKeepAlive(NPT_HttpMessage* message) 
{
    NPT_String protocol = message->GetProtocol();
    NPT_String connection;
    if (NPT_FAILED(message->GetHeaders().GetHeaderValue("Connection", connection))) {
        if (!protocol.Compare("HTTP/1.1", true))
            return true;

        return false;
    }

    // if we have the keep-alive header then no matter what protocol version we want keep-alive
    // if we are in HTTP 1.1 and we don't have the keep-alive header, make sure we also don't have the Connection: close header.
    if ((!protocol.Compare("HTTP/1.1", true) && connection.Compare("Close", true)) || !connection.Compare("keep-alive", true)) {
        return true; 
    }

    return false;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetHost
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetHost(NPT_HttpRequest* request, NPT_String& value)    
{ 
    return request->GetHeaders().GetHeaderValue("HOST", value);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetHost
+---------------------------------------------------------------------*/
void         
PLT_HttpHelper::SetHost(NPT_HttpRequest* request, const char* host)
{ 
    request->GetHeaders().SetHeader("HOST", host); 
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetRange(NPT_HttpRequest* request, NPT_Integer& start, NPT_Integer& end)
{
    start = -1;
    end = -1;    
    
    NPT_String range;
    if (NPT_FAILED(request->GetHeaders().GetHeaderValue("Range", range))) {
        return NPT_FAILURE;
    }

    char s[32], e[32];
    s[0] = '\0';
    e[0] = '\0';
    int ret = sscanf(range, "bytes=%[^-]-%s", s, e);
    if (ret < 1) {
        return NPT_FAILURE;
    }
    if (s[0] != '\0') {
        NPT_ParseInteger32(s, start);
    }
    if (e[0] != '\0') {
        NPT_ParseInteger32(e, end);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetRange
+---------------------------------------------------------------------*/
void
PLT_HttpHelper::SetRange(NPT_HttpRequest* request, NPT_Integer start, NPT_Integer end)
{
    NPT_String range = "bytes=";
    if (start != -1) {
        range += NPT_String::FromInteger(start);
    }
    range += '-';
    if (end != -1) {
        range += NPT_String::FromInteger(end);
    }
    request->GetHeaders().SetHeader("Range", range);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ToLog(NPT_HttpRequest* request, unsigned long level)
{
    //NPT_AutoLock lock(Lock);
    PLT_Log(level, "%s %s %s\r\n", (const char*)request->GetMethod(), (const char*)request->GetUrl().ToRequestString(true), (const char*)request->GetProtocol());
    return PLT_HttpHelper::ToLog((NPT_HttpMessage*)request, level);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetContentRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetContentRange(NPT_HttpResponse* response, NPT_Integer& start, NPT_Integer& end, NPT_Integer& length)
{
    NPT_String range;
    if (NPT_FAILED(response->GetHeaders().GetHeaderValue("Content-Range", range))) {
        return NPT_FAILURE;
    }

    start = -1;
    end = -1;
    length = -1;

    char s[32], e[32], l[32];
    s[0] = '\0';
    e[0] = '\0';
    l[0] = '\0';
    int ret = sscanf(range, "bytes %[^-]-%s[^/]/%s", s, e, l);
    if (ret < 3) {
        return NPT_FAILURE;
    }
    if (s[0] != '\0') {
        NPT_ParseInteger32(s, start);
    }
    if (e[0] != '\0') {
        NPT_ParseInteger32(e, end);
    }
    if (l[0] != '\0') {
        NPT_ParseInteger32(l, length);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetContentRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetContentRange(NPT_HttpResponse* response, NPT_Integer start, NPT_Integer end, NPT_Integer length)
{
    if (start < 0 || end < 0 || length < 0) {
        PLT_Log(PLT_LOG_LEVEL_1, "ERROR: Invalid Content Range!\n");
        return NPT_FAILURE;
    }

    NPT_String range = "bytes ";
    range += NPT_String::FromInteger(start);
    range += '-';
    range += NPT_String::FromInteger(end);
    range += '/';
    range += NPT_String::FromInteger(length);
    response->GetHeaders().SetHeader("Content-Range", range);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ToLog(NPT_HttpResponse* response, unsigned long level)
{
    //NPT_AutoLock lock(Lock);
    PLT_Log(level, "%s %d %s\r\n", (const char*)response->GetProtocol(), response->GetStatusCode(), (const char*)response->GetReasonPhrase());
    return PLT_HttpHelper::ToLog((NPT_HttpMessage*)response, level);
}

/*----------------------------------------------------------------------
|   PLT_HttpClient::Connect
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClient::Connect(NPT_Socket* connection, NPT_HttpRequest& request, NPT_Timeout timeout)
{
    // get the address of the server
    NPT_IpAddress server_address;
    NPT_CHECK(server_address.ResolveName(request.GetUrl().GetHost(), timeout));
    NPT_SocketAddress address(server_address, request.GetUrl().GetPort());

    // connect to the server
    NPT_Debug("CreateConnection: will connect to %s:%d\n", (const char*)request.GetUrl().GetHost(), request.GetUrl().GetPort());
    NPT_CHECK(connection->Connect(address, timeout));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpClient::SendRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClient::SendRequest(NPT_OutputStreamReference& output_stream, 
                            NPT_HttpRequest&           request, 
                            NPT_Timeout                timeout)
{
    NPT_COMPILER_UNUSED(timeout);

    // add any headers that may be missing
    NPT_HttpHeaders& headers = request.GetHeaders();
    //headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");
    if (!headers.GetHeader(NPT_HTTP_HEADER_USER_AGENT)) {
        headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, 
            "Platinum/" PLT_PLATINUM_VERSION_STRING);
    }

    // set host only if not already set
    if (!headers.GetHeader(NPT_HTTP_HEADER_HOST)) {
        NPT_String host = request.GetUrl().GetHost();
        if (request.GetUrl().GetPort() != NPT_HTTP_DEFAULT_PORT) {
            host += ":";
            host += NPT_String::FromInteger(request.GetUrl().GetPort());
        }
        headers.SetHeader(NPT_HTTP_HEADER_HOST, host);
    }

    // get the request entity to set additional headers
    NPT_InputStreamReference body_stream;
    NPT_HttpEntity* entity = request.GetEntity();
    if (entity && NPT_SUCCEEDED(entity->GetInputStream(body_stream))) {
        // content length
        headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, 
            NPT_String::FromInteger(entity->GetContentLength()));

        // content type
        NPT_String content_type = entity->GetContentType();
        if (!content_type.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_TYPE, content_type);
        }

        // content encoding
        NPT_String content_encoding = entity->GetContentEncoding();
        if (!content_encoding.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_ENCODING, content_encoding);
        }
    } else {
        // force content length to 0 is there is no message body
        headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, "0");
    }

    // create a memory stream to buffer the headers
    NPT_MemoryStream header_stream;

    // emit the request headers into the header buffer
    request.Emit(header_stream);

    // send the headers
    NPT_CHECK(output_stream->WriteFully(header_stream.GetData(), header_stream.GetDataSize()));

    // send request body
    if (!body_stream.IsNull()) {
        NPT_CHECK(NPT_StreamToStreamCopy(*body_stream.AsPointer(), *output_stream.AsPointer()));
    }

    // flush the output stream so that everything is sent to the server
    return output_stream->Flush();
}

/*----------------------------------------------------------------------
|   PLT_HttpClient::WaitForResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClient::WaitForResponse(NPT_InputStreamReference& input_stream,
                                NPT_HttpRequest&          request, 
                                NPT_SocketInfo&           info, 
                                NPT_HttpResponse*&        response)
{
    NPT_COMPILER_UNUSED(info);

    // create a buffered stream for this connection stream
    NPT_BufferedInputStreamReference buffered_input_stream(new NPT_BufferedInputStream(input_stream));

    // parse the response
    NPT_CHECK(NPT_HttpResponse::Parse(*buffered_input_stream, response));

    // unbuffer the stream
    buffered_input_stream->SetBufferSize(0);

    // decide what to do next based on the response 
    //switch (response->GetStatusCode()) {
    //    // redirections
    //    case NPT_HTTP_STATUS_301_MOVED_PERMANENTLY:
    //    case NPT_HTTP_STATUS_302_FOUND:
    //}

    // create an entity if one is expected in the response
    if (request.GetMethod() == NPT_HTTP_METHOD_GET || request.GetMethod() == NPT_HTTP_METHOD_POST) {
        NPT_HttpEntity* response_entity = new NPT_HttpEntity(response->GetHeaders());
        response_entity->SetInputStream((NPT_InputStreamReference)buffered_input_stream);
        response->SetEntity(response_entity);
    }

    return NPT_SUCCESS;
}
