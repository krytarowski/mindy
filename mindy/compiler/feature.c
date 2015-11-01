/**********************************************************************\
*
*  Copyright (c) 1995, 1996  Carnegie Mellon University
*  Copyright (c) 1998, 1999, 2000  Gwydion Dylan Maintainers
*  All rights reserved.
*
*  Use and copying of this software and preparation of derivative
*  works based on this software are permitted, including commercial
*  use, provided that the following conditions are observed:
*
*  1. This copyright notice must be retained in full on any copies
*     and on appropriate parts of any derivative works.
*  2. Documentation (paper or online) accompanying any system that
*     incorporates this software, or any part of it, must acknowledge
*     the contribution of the Gwydion Project at Carnegie Mellon
*     University, and the Gwydion Dylan Maintainers.
*
*  This software is made available "as is".  Neither the authors nor
*  Carnegie Mellon University make any warranty about the software,
*  its performance, or its conformity to any specification.
*
*  Bug reports should be sent to <gd-bugs@gwydiondylan.org>; questions,
*  comments and suggestions are welcome at <gd-hackers@gwydiondylan.org>.
*  Also, see http://www.gwydiondylan.org/ for updates and documentation.
*
***********************************************************************
*
* This file handles conditional compilation
*
\**********************************************************************/

#include "mindycomp.h"
#include "feature.h"
#include "lexer.h"
#include "src.h"
#include "parser-tab.h"
#include "sym.h"


static struct feature {
    struct symbol *symbol;
    struct feature *next;
} *Features = NULL;

static const char *InitialFeatures[]
  = {"mindy",
#    ifdef _WIN32
        "compiled-for-x86",
        "compiled-for-win32",
#    else
        "compiled-for-unix",
#    endif
     NULL};

static bool feature_present(struct symbol *sym)
{
    struct feature *feature;

    for (feature = Features; feature != NULL; feature = feature->next)
        if (feature->symbol == sym)
            return true;

    return false;
}

static struct state {
    int line;
    bool active;
    bool do_else;
    bool seen_else;
    struct state *old_state;
} *State = NULL;

static void push_state(bool active, bool do_else)
{
    struct state *new = malloc(sizeof(struct state));

    new->line = line_count;
    new->active = active;
    new->do_else = do_else;
    new->seen_else = false;
    new->old_state = State;
    State = new;
}

static void pop_state(void)
{
    struct state *old = State->old_state;

    free(State);

    State = old;
}


static int yytoken = 0;

static void new_token(void)
{
    free(yylval.token);
    yytoken = internal_yylex();
}


static void parse_error(void)
{
    if (yytoken)
        error(line_count,
              "syntax error in feature condition at or before ``%s''",
              yylval.token->chars);
    else
        error(line_count, "syntax error in feature condition at end-of-file");
    exit(1);
}

static bool parse_feature_expr(void);

static bool parse_feature_word(void)
{
    struct symbol *sym;

    if (yylval.token->chars[0] == '\\')
        sym = symbol((char *)yylval.token->chars + 1);
    else
        sym = symbol((char *)yylval.token->chars);

    new_token();

    return feature_present(sym);
}

static bool parse_feature_term(void)
{
    switch (yytoken) {
      case LPAREN:
        return parse_feature_expr();

      case TILDE:
        new_token();
        return !parse_feature_term();

        /* All the various things that look like words. */
      case ABSTRACT: case ABOVE: case DBEGIN: case BELOW: case BLOCK:
      case BY: case CASE: case CLASS: case CLEANUP: case CONCRETE:
      case CONSTANT: case DEFINE: case DOMAIN: case EACH_SUBCLASS: case ELSE:
      case ELSEIF: case END: case EXCEPTION: case FINALLY: case FOR:
      case FREE: case FROM: case GENERIC: case HANDLER: case IF: case IN:
      case INHERITED: case INSTANCE: case KEYED_BY: case KEYWORD_RESERVED_WORD:
      case LET: case LOCAL: case METHOD: case OPEN: case OTHERWISE:
      case PRIMARY: case REQUIRED: case SEALED: case SELECT: case SLOT:
      case THEN: case TO: case UNLESS: case UNTIL: case VARIABLE: case VIRTUAL:
      case WHILE: case MODULE: case LIBRARY: case EXPORT: case CREATE:
      case USE: case ALL: case SYMBOL:
        return parse_feature_word();

      default:
        parse_error();
        return 0;
    }
}

static bool parse_feature_expr(void)
{
    int res;

    /* Consume the left paren. */
    new_token();

    res = parse_feature_term();

    while (1) {
        switch (yytoken) {
          case RPAREN:
            /* Consume the right paren and return. */
            new_token();
            return res;

          case BINARY_OPERATOR:
            switch (yylval.token->chars[0]) {
              case '&':
                new_token();
                if (!parse_feature_term())
                    res = false;
                break;

              case '|':
                new_token();
                if (parse_feature_term())
                    res = true;
                break;

              default:
                parse_error();
            }
            break;

          default:
            parse_error();
        }
    }
}

static bool parse_conditional(void)
{
    /* Consume the #if or #elseif token */
    new_token();

    /* If the next token isn't a right paren, something is wrong. */
    if (yytoken != LPAREN)
        parse_error();

    return parse_feature_expr();
}

int yylex(void)
{
    bool cond;

    yytoken = internal_yylex();

    while (1) {
        switch (yytoken) {
          case FEATURE_IF:
            cond = parse_conditional();
            if (State == NULL || State->active)
                push_state(cond, !cond);
            else
                push_state(false, false);
            break;

          case FEATURE_ELSE_IF:
            if (State == NULL) {
                error(line_count,
                      "#elseif with no matching #if, treating as #if");
                cond = parse_conditional();
                push_state(cond, !cond);
            }
            else if (State->seen_else) {
                error(line_count, "#elseif after #else in one #if");
                State->active = false;
            }
            else if (parse_conditional()) {
                State->active = State->do_else;
                State->do_else = false;
            }
            else
                State->active = false;
            break;

          case FEATURE_ELSE:
            if (State == NULL)
                error(line_count, "#else with no matching #if, ignoring");
            else if (State->seen_else) {
                error(line_count, "#else after #else in one #if");
                State->active = false;
            }
            else {
                State->seen_else = true;
                State->active = State->do_else;
            }
            new_token();
            break;

          case FEATURE_ENDIF:
            if (State == NULL)
                error(line_count, "#endif with no matching #if, ignoring");
            else
                pop_state();
            new_token();
            break;

          case 0:
            while (State != NULL) {
                error(State->line,
                      "#if with no matching #endif, assuming #endif at EOF.");
                pop_state();
            }
            return yytoken;

          default:
            if (State == NULL || State->active)
                return yytoken;
            new_token();
            break;  /* Break out of switch */
        }
    }
}

void add_feature(struct symbol *sym)
{
    if (!feature_present(sym)) {
        struct feature *new = malloc(sizeof(struct feature));
        new->symbol = sym;
        new->next = Features;
        Features = new;
    }
}

void remove_feature(struct symbol *sym)
{
    struct feature *feature, **ptr;

    for (ptr = &Features; (feature = *ptr) != NULL; ptr = &feature->next) {
        if (feature->symbol == sym) {
            *ptr = feature->next;
            free(feature);
            return;
        }
    }
}

void init_feature(void)
{
    const char **ptr;

    for (ptr = InitialFeatures; *ptr != NULL; ptr++)
        add_feature(symbol(*ptr));
}

