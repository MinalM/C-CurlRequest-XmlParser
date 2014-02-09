/*
* Copyright (c) 2014-2015, Minal Mishra
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of the Weather Channel nor the names of the
* contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MICHAEL JOHN BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

struct MemoryStruct{
    char *memory;
    size_t size;
};

static size_t write2MemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct*)userp;
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
    {
        /* out of memory */
        printf("not enough memory (realloc returned)\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

//string token replace
char *replace_str(char *str, char *orig, char *rep)
{
  char buffer[4096];
  char *p;
  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

//send http request using curl and store the response in a memory struct
void sendRequest(const char *url, struct MemoryStruct *chunk)
{
    CURL *curl_handle;
    
    //    struct MemoryStruct chunk;
    chunk->memory = malloc(1);
    chunk->size = 0;
    
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &write2MemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
    //insecure request
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    printf("\nSending HTTPS Request %s \n",url);
    curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);
    printf("SIZEOF(response) %d",chunk->size);
    printf("\nRESPONSE BODY\n --------\n %s \n---------\n", chunk->memory);
    //return 0;
}

//read http response and load it into libxml xmlDocPtr
xmlDocPtr readXmlInMemory(const char *data, int sizeData)
{
  LIBXML_TEST_VERSION
    xmlDocPtr doc;
  doc = xmlParseMemory(data, sizeData);
  if(doc == NULL){
    fprintf(stderr, "Failed to parse response \n");
  }
  return (doc);
}

//get Xpath Context to query
xmlXPathContextPtr getXPathContext(xmlDocPtr doc)
{
  return (xmlXPathNewContext(doc));
}

//provide xpath query to return a nodes matching what we are looking for
xmlXPathObjectPtr
getnodeset (xmlXPathContextPtr context, xmlChar *xpath){
	
	xmlXPathObjectPtr result;

	if (context == NULL) {
		printf("Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		printf("Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
                printf("No result\n");
		return NULL;
	}
	return result;
}

//return xmlnode text value using xpath query for the xmlnode
char *parseResponse(struct MemoryStruct* chunk, char* node)
{
    xmlDocPtr doc = readXmlInMemory(chunk->memory,chunk->size);
    xmlXPathContextPtr context = getXPathContext(doc);
    xmlChar *xpath = (xmlChar*) node;//change the node for which you need the value
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    int i;
    xmlChar *keyword;
    char *word;
    result = getnodeset (context, xpath);
    if (result) {
      nodeset = result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
	keyword = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
	printf("\nXpath query response for\n %s: %s\n", xpath, keyword);
	word = (char *)keyword;
	//xmlFree(keyword);
      }
      xmlXPathFreeObject (result);
    }
    xmlFreeDoc(doc);
    return word;
}

int main ( int argc, char *argv[] )
{
    char *urlinit = "http://rxnav.nlm.nih.gov/REST/RxTerms/rxcui/198440/allinfo";

    curl_global_init(CURL_GLOBAL_ALL);
    struct MemoryStruct chunk; 

    //GET GenericName from response
    sendRequest(urlinit, &chunk);
    char* retVal = parseResponse(&chunk, "//fullGenericName");
    if (chunk.memory)
        free(chunk.memory);

    xmlCleanupParser();
    curl_global_cleanup();
    
    return 0;
}	/* ---------- end of function main ---------- */
