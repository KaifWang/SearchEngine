#pragma once

// Machine communication configs
#define DOCUMENT_MESSAGE_DELIMITER '@'
#define NUM_INDEX_MACHINE 8
#define INDEX_IP_FILE "indexChunkIP.txt"

// Index configs
#define INPUT_DIRECTORY "parsed_output"
#define INDEX_DIRECTORY "indexChunk"
#define FINAL_INDEX "demoIndex"
#define NUM_DOCS_IN_DICT 30000
#define DOC_DATA_SPARSE_FILE_SIZE 8000000
#define TOKEN_MAX_SIZE 20
#define MAX_TITLE_SIZE 50
#define MAX_ABSTRACT_SIZE 100

// Ranker configs
#define NUM_TOP_DOCUMENTS_RETURNED 30
#define MAX_DOCUMENT_PER_INDEX_PER_QUERY 1000

// FrontEnd configs
#define FRONT_END_IP "54.160.96.18"
#define MAX_NUM_CONCURRENT_CONNECTION 1000
#define MAX_QUERY_SIZE 40
#define RESULTS_PER_PAGE 10
#define SEARCH_INDEX_TIMEOUT 60