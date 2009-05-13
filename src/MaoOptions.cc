//
// Copyright 2008 Google Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301, USA.

#include <list>
#include <string>
#include <utility>
#include <vector>

#include <stdio.h>
#include <ctype.h>
#include "MaoDebug.h"
#include "MaoOptions.h"
#include "MaoPasses.h"
#include "MaoPlugin.h"

// Maintain static list of all option arrays.
typedef std::vector<MaoOptionArray *> OptionVector;
static OptionVector *option_array_list;

// Unprocessed flags are passed on to as_main (which is the GNU Assembler
// main function). Everything else is handed to the MAO Option processor
//
void MaoOptions::ProvideHelp(bool always) {
  if (!help() && !always) return;

  fprintf(stderr,
          "Mao %s\n",
          MAO_VERSION);
  fprintf(stderr,
          "Usage: mao [--mao=mao-options]* "
          "[regular-assembler-options]* input-file \n"
          "\n'mao-options' are seperated by commas, and are:\n\n"
          "-h          display this help text\n"
          "-v          verbose (set trace level to 3)\n"
          "-T          output timing information for passes\n"
          "--plugin    load the specified plugin\n"
          "\n"
          "PASS=[phase-option][,phase-option]*\n"
          "\nwith PASS and 'phase-option' being:\n\n"
          "Pass: ALL\n"
          "  trace     : (int)    Set trace level to 'val' (0..3)\n"
          "  db[parm]  : (bool)   Dump before a pass\n"
          "  da[parm]  : (bool)   Dump after  a pass\n"
          "     with parm being one of:\n"
          "        cfg : dump CFG, if available\n"
          "        vcg : dump VCG file, if CFG is available\n"
          );

  for (OptionVector::iterator it = option_array_list->begin();
       it != option_array_list->end(); ++it) {
    fprintf(stderr, "Pass: %s\n", (*it)->name());
    MaoOption *arr = (*it)->array();
    for (int i = 0; i < (*it)->num_entries(); i++) {
      fprintf(stderr, "  %-10s: %7s %s\n",
              arr[i].name(),
              arr[i].type() == OPT_INT ? "(int)   " :
              arr[i].type() == OPT_BOOL ? "(bool)  " :
              arr[i].type() == OPT_STRING ? "(string)" :
              "(unk)",
              arr[i].description());
    }
  }
  exit(0);
}

void MaoOptions::TimerPrint() {
  double total_secs = 0.0;
  for (OptionVector::iterator it1 = option_array_list->begin();
       it1 != option_array_list->end(); ++it1) {
    total_secs += (*it1)->timer()->GetSecs();
  }

  fprintf(stderr, "Timing information for passes\n");
  for (OptionVector::iterator it = option_array_list->begin();
       it != option_array_list->end(); ++it) {
    if ((*it)->timer()->Triggered()) {
      fprintf(stderr, "  Pass: %-12s %5.1lf [sec] %5.1lf%%\n",
	      (*it)->name(), (*it)->timer()->GetSecs(),
	      100.0 * (*it)->timer()->GetSecs() / total_secs);
    }
  }
  fprintf(stderr, "Total accounted for: %5.1lf [sec]\n", total_secs );
}


// Simple (static) constructor to allow registration of
// option arrays.
//
MaoOptionRegister::MaoOptionRegister(const char *name,
                                     MaoOption  *array,
                                     int N,
                                     MaoTimer   *timer) {
  MaoOptionArray *tmp = new MaoOptionArray(name, array, N, timer);
  if (!option_array_list) {
    option_array_list = new OptionVector;
  }
  option_array_list->push_back(tmp);
}

static MaoOptionArray *FindOptionArray(const char *pass_name) {
  for (OptionVector::iterator it = option_array_list->begin();
       it != option_array_list->end(); ++it) {
    if (!strcasecmp((*it)->name(), pass_name))
      return (*it);
  }
  MAO_ASSERT_MSG(false, "Can't find passname: %s", pass_name);
  return NULL;
}

void MaoOptions::SetOption(const char *pass_name,
                           const char *option_name,
                           int         value) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  MaoOption      *option = entry->FindOption(option_name);
  MAO_ASSERT(option->type() == OPT_INT);

  option->value.ival_ = value;
}

void MaoOptions::SetOption(const char *pass_name,
                           const char *option_name,
                           bool        value) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  MaoOption      *option = entry->FindOption(option_name);
  MAO_ASSERT(option->type() == OPT_BOOL);

  option->value.bval_ = value;
}

void MaoOptions::SetOption(const char *pass_name,
                           const char *option_name,
                           const char *value) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  MaoOption      *option = entry->FindOption(option_name);
  MAO_ASSERT(option->type_ == OPT_STRING);

  option->value.cval_ = value;
}

void MaoOptions::TimerStart(const char *pass_name) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  entry->timer()->Start();
}

void MaoOptions::TimerStop(const char *pass_name) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  entry->timer()->Stop();
}

static char *NextToken(const char *arg, const char **next, char *token_buff) {
  int i = 0;
  const char *p = arg;
  if (*p == ',' || *p == ':' || *p == '=' || *p == '+')
    ++p;
  while (isalnum(*p) || (*p == '_') ||
         (*p == '/') ||
         (*p == '.') || (*p == '-'))
    token_buff[i++] = *p++;

  token_buff[i]='\0';
  *next = p;
  return token_buff;
}

static const char *GobbleGarbage(const char *arg, const char **next) {
  const char *p = arg;
  if (*p == ',' || *p == ':' || *p == '=')
    ++p;
  *next = p;
  return p;
}

// At the current parameter location, check if we have
// a parameter, e.g.:
//    option:val
//    option(val)
//    option[val]
//
static bool GetParam(const char *arg, const char **next, const char **param,
                     char *token_buff) {
  if (arg[0] == '(' || arg[0] == '[') {
    char  delim = arg[0];

    ++arg;
    *param = NextToken(arg, &arg, token_buff);

    if (delim != ':') {
      MAO_ASSERT_MSG(arg[0] == ')' || arg[0] == ']',
                     "Ill formatted parameter to option: %s", arg);
      ++arg;  // skip closing bracket
    }
    *next = arg;
    return true;
  } else {
    *next = arg;
    *param = NULL;
    return false;
  }
}

// See whether an option is a "pass-specific" options, an option
// that applies to passes, but is not specified in the pass' option
// array.
//
bool SetPassSpecificOptions(const char *option, const char *arg,
                            const char **next, char *token_buff,
                            MaoOptionMap *options) {
  MaoOptionValue value;

  if (!strcasecmp(option, "trace")) {
    const char *param;
    value.ival_ = 1;
    if (GetParam(arg, next, &param, token_buff))
      value.ival_ = atoi(param);
    (*options)["trace"] = value;
    return true;
  }
  if (!strcasecmp(option, "db") ||
      !strcasecmp(option, "da")) {
    const char *param;
    GetParam(arg, next, &param, token_buff);
    if (param) {
      std::string full_option = option;
      full_option.append("[").append(param).append("]");
      value.bval_ = true;
      (*options)[full_option] = value;
    }
    return true;
  }
  return false;
}

void MaoOptions::InitializeOptionMap(MaoOptionMap *options,
                                     MaoOptionArray *pass_opts) {
  // Initialize builtin options
  (*options)["trace"].ival_ = verbose() ? 3 : 0;
  (*options)["da[vcg]"].bval_ = 0;
  (*options)["db[vcg]"].bval_ = 0;
  (*options)["da[cfg]"].bval_ = 0;
  (*options)["db[cfg]"].bval_ = 0;

  // Initialize the options with default values
  for (int i = 0; i < pass_opts->num_entries(); i++) {
    (*options)[pass_opts->array()[i].name()] = pass_opts->array()[i].value;
  }
}

// Reparse the accumulated option strings. The reason for reparsing is
// that dynamically created passes are not visible at standard option
// parsing time. We therefore reparse on pass creation.
//
void MaoOptions::Reparse(MaoUnit *unit, MaoPassManager *pass_man) {
  Parse(mao_options_, false, unit, pass_man);
}

void MaoOptions::Parse(const char *arg, bool collect,
                       MaoUnit *unit, MaoPassManager *pass_man) {
  MaoFunctionPassManager *func_pass_man = NULL;

  // Initialize the options for all static option passes
  for (RegisteredStaticOptionPassMap::const_iterator pass =
           GetStaticOptionPasses().begin();
       pass != GetStaticOptionPasses().end(); ++pass) {
    MaoOptionArray *current_opts = FindOptionArray(pass->first);
    MAO_ASSERT(current_opts);
    InitializeOptionMap(pass->second, current_opts);
  }

  if (!arg) return;

  if (collect) {
    if (!mao_options_) {
      mao_options_ = strdup(arg);
    } else {
      // Append mao_options_ and arg
      char *buf = (char *)malloc(sizeof(char) * (strlen(mao_options_) +
                                                 strlen(arg) +
                                                 1 +
                                                 1));
      sprintf(buf, "%s:%s", mao_options_, arg);
      free(mao_options_);
      mao_options_ = buf;
    }
  }

  char *token_buff = new char[strlen(arg)+1];
  while (arg && arg[0]) {
    GobbleGarbage(arg, &arg);

    // Standard options start with a '-'.
    //
    if (arg[0] == '-') {
      ++arg;
      if (arg[0] == 'v') {
        set_verbose();
        ++arg;
      } else if (arg[0] == 'h') {
        set_help();
        ++arg;
      } else if (arg[0] == 'T') {
        set_timer_print();
        ++arg;
      } else if (!strncmp(arg, "-plugin", 7)) {
        arg += 7;
        GobbleGarbage(arg, &arg);
        char *plugin = NextToken(arg, &arg, token_buff);
        if (collect)
          LoadPlugin(plugin);
      } else {
        fprintf(stderr, "Invalid Option starting with: %s\n", arg);
        ++arg;
      }
      continue;
    }

    // Named passes start with a regular character,
    // have an identifier (a valid pass name), and
    // are followed by either '=' or ':'.
    //
    if (isascii(arg[0])) {
      std::string pass_name(NextToken(arg, &arg, token_buff));
      if (pass_name.size() != 0) {
        MaoOptionArray *current_opts = FindOptionArray(pass_name.c_str());
        MAO_ASSERT(current_opts);

        bool static_option_pass = true;
        MaoOptionMap *options = GetStaticOptionPass(pass_name.c_str());
        if (!options) {
          options = new MaoOptionMap;
          InitializeOptionMap(options, current_opts);
          static_option_pass = false;
        }

        if (arg[0] == '=') {
          char *option;
          while (1) {
            const char *old_arg = arg;
            // should we start a new parse group?
            if (arg[0] == ':')
              break;

            option = NextToken(arg, &arg, token_buff);
            if (!option || option[0] == '\0')
              break;

            // if the option is a passname, the next character will be a '='
            // Then we need to exit the option parsing and process the next PASS
            //
            if (arg[0] == '=') {
              arg = old_arg;
              break;
            }
            if (SetPassSpecificOptions(option, arg, &arg, token_buff, options))
              continue;

            MaoOption *opt = current_opts->FindOption(option);
            MAO_ASSERT_MSG(opt, "Could not find option: %s", option);

            const char *param;
            if (GetParam(arg, &arg, &param, token_buff)) {
              if (opt->type() == OPT_INT)
                (*options)[opt->name()].ival_ = atoi(param);
              else if (opt->type() == OPT_STRING)
                (*options)[opt->name()].cval_ = strdup(param);
              else if (opt->type() == OPT_BOOL) {
                if (param[0] == '0' ||
                    param[0] == 'n' || param[0] == 'N' ||
                    param[0] == 'f' || param[0] == 'F' ||
                    (param[0] == 'o' && param[1] == 'f'))
                  (*options)[opt->name()].bval_ = false;
                else if (param[0] == '1' ||
                         param[0] == 'y' || param[0] == 'Y' ||
                         param[0] == 't' || param[0] == 'T' ||
                         (param[0] == 'o' && param[1] == 'n'))
                  (*options)[opt->name()].bval_ = true;
              }
            } else {
              if (opt->type() == OPT_BOOL)
                (*options)[opt->name()].bval_ = true;
              else
                MAO_ASSERT_MSG(false, "non-boolean option %s used as boolean",
                               option);
            }
            if (arg[0] == '\0') break;
            if (arg[0] == ':' || arg[0] == '|' || arg[0] == ';') {
              ++arg;
              break;
            }
            ++arg;
          }
          GobbleGarbage(arg, &arg);
        }

        if (pass_man) {
          MaoPassManager::PassCreator unit_creator =
              GetUnitPass(pass_name.c_str());
          if (unit_creator) {
            pass_man->LinkPass(unit_creator(options, unit));
            func_pass_man = NULL;
          } else {
            MaoFunctionPassManager::PassCreator func_creator =
                GetFunctionPass(pass_name.c_str());
            if (func_creator) {
              if (!func_pass_man) {
                MaoOptionMap *func_options = new MaoOptionMap;
                InitializeOptionMap(func_options, FindOptionArray("PASSMAN"));
                func_pass_man = new MaoFunctionPassManager(func_options, unit);
                pass_man->LinkPass(func_pass_man);
              }
              func_pass_man->LinkPass(std::make_pair(func_creator, options));
            } else if (static_option_pass) {
              // Nothing to do for static option passes
            } else {
              delete options;
              MAO_ASSERT_MSG(false, "Options for non-pass found: %s",
                             pass_name.c_str());
            }
          }
        } else if (!static_option_pass) {
          // We're not constructing passes now, so just discard the
          // options.
          delete options;
        }

        continue;
      }
    }

    fprintf(stderr, "Unknown input: %s\n", arg);
    ++arg;
  }
  delete [] token_buff;
}
