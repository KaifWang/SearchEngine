//
// crawler_cfg.h
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Configuration file for all crawler part
//

#pragma once

// Checkpoint
#define UMSE_CRAWLER_FRONTIER_CHECKPOINT "./checkpoints/frontier.txt"
#define UMSE_CRAWLER_SEENSET_CHECKPOINT "./checkpoints/seenset.txt"
#define UMSE_CRAELER_ROBOTTXT_CEHCKPOINT "./checkpoints/robottxt.txt"
#define UMSE_CRAWLER_CHECK_ALIVE_CHECKPOINT "./checkpoints/checkalive.txt"

// Output folder
#define UMSE_CRAWLER_PARSED_OUTPUT_FOLDER "./outputs/parsed_output/"
#define UMSE_CRAWLER_HTMLDOWNLOAD_OUTPUT_FOLDER "./outputs/download_output/"
// #define UMSE_CRAWLER_SAVE_HTMLOUTPUT // comment off to disable save html

// Settings - Crawler Core
#define UMSE_CRAWLER_NUM_CONCURRENT 400                                  // number of concurrent download, parse, send thread
#define UMSE_CRAWLER_END_SIGNAL "**endsignal**"                          // end signal to send to machine
#define UMSE_CRAWLER_LISTEN_MSG_PORT "5000"                              // port to listen new url send from other machine
#define UMSE_CRAWLER_LISTEN_SIGNAL_PORT "5002"                           // port to listen end signal
#define UMSE_CRAWLER_DEBUG_PORT "5004"                                   // port for debug
#define UMSE_CRAWLER_NUM_MACHINE 10                                       // number of machine
#define UMSE_CRAWLER_HOSTNAMES_FILENAME "./configs/machine_hostname.txt" // file that store all machine's host address

//#define UMSE_CRAWLER_DEBUG_SINGLE_MACHINE_SEND_ITSELF // single machine that only send new url to itself
//#define UMSE_CRAWLER_DEBUG_SINGLE_MACHINE_SEND_5004  // single machine that do not send new url to itself

// Seetings - frontier
#define UMSE_CRAWLER_FRONTIER_MAX_SIZE 1000000      // Max Size limitation to avoid malloc failure
#define UMSE_CRAWLER_FRONTIER_TOPK_RATIO 3         // R in the frontier

// Log
#define _UMSE_CRAWLER_CRAWLERCORE_LOG     // corr to crawler_core.hpp
//#define _UMSE_CRAWLER_FRONTIER_LOG        // corr to html_frontier.hpp
//#define _UMSE_CRAWLER_ROBOTXT_LOG         // corr to html_filter.hpp
//#define _UMSE_CRAWLER_SEENSET_LOG         // corr to html_filter.hpp
//#define _UMSE_CRAWLER_FILTER_LOG          // corr to html_filter.hpp
#define _UMSE_CRAWLER_NETWORK_UTILITY_LOG // corr to html_download.hpp
//#define _UMSE_CRAWLER_RESULT_HANDLER_LOG  // corr to parsed_result_handler.hpp
//#define _UMSE_RAII_LOG
//#define _UMSE_CRAWLER_EVAL_URL_LOG
//#define _UMSE_CRAWLER_HTMLPARSER_LOG          // corr to html_parser.hpp
//#define _UMSE_CRAWLER_PARSEDURL_LOG           // corr to url_parser.hpp

// assert
#define _UMSE_CRAWLER_HTMLPARSER_ASSERT

// STL choice
#include "../../utility/string.h"
#include "../../utility/vector.h"
#include "../../utility/strstr.hpp"
//using umse::string;
//using umse::vector;

