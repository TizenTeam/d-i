/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: commands.c
 *
 * Description: implementation of each command specified in the spec
 *
 * $Id: commands.c,v 1.31 2002/11/23 00:37:20 barbier Exp $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "commands.h"
#include "frontend.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "strutl.h"

#define CHECKARGC(pred) \
	if (_command_checkargc(argc pred, out, outsize) == DC_NOTOK) \
		return DC_OK


/*
 * Function: _command_checkargc
 * Input: int pred - predicate
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_OK if pred is true, DC_NOTOK otherwise
 * Description: Checks to see if a given predicate is true; and if not
 *              write an appropriate message into the output buffer.
 *              Used in conjunction with the CHECKARGC macro above
 * Assumptions: static, to be used by command_* functions below
 */
static int _command_checkargc(int pred, char *out, size_t outsize)
{
	if (!pred)
	{
		snprintf(out, outsize, "%u Incorrect number of arguments", 
			CMDSTATUS_SYNTAXERROR);
		return DC_NOTOK;
	}
	else
	{
		return DC_OK;
	}
}

/*
 * Function: command_input
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the INPUT debconf command; adds a question
 *              to the list of questions to be asked if appropriate.
 * Assumptions: none
 */
int command_input(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	int visible = 1;
	struct question *q = 0;
	char *qtag;
	char *priority;

	CHECKARGC(== 2);

	priority = argv[1];
	qtag = argv[2];

	/* check priority */
	visible = (mod->frontend->interactive && 
               mod->questions->methods.is_visible(mod->questions, qtag, priority));

	if (visible)
	{
		q = mod->questions->methods.get(mod->questions, qtag);
		if (q == NULL) 
		{
			snprintf(out, outsize, "%u No such question",
				CMDSTATUS_BADQUESTION);
			return DC_OK;
		}
		visible = mod->frontend->methods.add(mod->frontend, q);
	}

	if (visible) 
		snprintf(out, outsize, "%u Question will be asked",
			 CMDSTATUS_SUCCESS);
	else
		snprintf(out, outsize, "%u Question skipped",
			CMDSTATUS_INPUTINVISIBLE);
        question_deref(q);
	return DC_OK;
}

/*
 * Function: command_clear
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the CLEAR debconf command; removes any 
 *              questions currently in the queue
 * Assumptions: none
 */
int command_clear(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	CHECKARGC(== 0);

	mod->frontend->methods.clear(mod->frontend);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

/*
 * Function: command_version
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the VERSION debconf command; checks to see
 *              if the version required by a confmodule script is 
 *              compatible with the debconf version we recognize
 * Assumptions: none
 */
int command_version(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	int ver;

	CHECKARGC(== 1);

	ver = atoi(argv[1]);

	if (ver < DEBCONF_VERSION)
		snprintf(out, outsize, "%u Version too low (%d)",
			CMDSTATUS_BADVERSION, ver);
	else if (ver > DEBCONF_VERSION)
		snprintf(out, outsize, "%u Version too high (%d)",
			CMDSTATUS_BADVERSION, ver);
	else
		snprintf(out, outsize, "%u %.0f", CMDSTATUS_SUCCESS,
			DEBCONF_VERSION);
	return DC_OK;
}

/*
 * Function: command_capb
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the CAPB debconf command; exchanges 
 *              capability information between the confmodule and the
 *              frontend
 * Assumptions: none
 */
int command_capb(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	/* TODO: tell frontend about confmodule capabilities and return
	 * frontend capability to the confmodule 
	 */
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

/*
 * Function: command_title
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the TITLE debconf command; sets the title in 
 *              the frontend
 * Assumptions: none
 */
int command_title(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
        char *buf;
        size_t bufsize = 1024;
	int i;

        buf = malloc(bufsize);
        if (!buf)
                return DC_NOTOK;
        memset(buf, 0, bufsize);
	for (i = 1; i <= argc; i++) {
                while ((strlen(buf) + strlen(argv[i]) + 1) > bufsize) {
                        bufsize += 1024;
                        buf = realloc(buf, bufsize);
                        if (!buf)
                                return DC_NOTOK;
                }
		strvacat(buf, bufsize, argv[i], " ", 0);
        }

	mod->frontend->methods.set_title(mod->frontend, buf);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

/*
 * Function: command_beginblock
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf BEGINBLOCK command; not yet 
 *              implemented
 * Assumptions: TODO
 */
int command_beginblock(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

/*
 * Function: command_endblock
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf ENDBLOCK command; not yet
 *              implemented
 * Assumptions: TODO
 */
int command_endblock(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

/*
 * Function: command_go
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf GO command; asks all pending 
 *              questions and save the answers in the debconf DB
 * Assumptions: frontend will return CMDSTATUS_GOBACK only if the
 *              confmodule supports backing up
 */
int command_go(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;

	CHECKARGC(== 0);
	if (mod->frontend->methods.go(mod->frontend) == CMDSTATUS_GOBACK)
		snprintf(out, outsize, "%u backup", CMDSTATUS_GOBACK);
	else {
		snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
		/* FIXME  questions should be tagged when closing session */
		for (q = mod->frontend->questions; q != NULL; q = q->next)
			q->flags |= DC_QFLAG_SEEN;
        }
	mod->frontend->methods.clear(mod->frontend);

	return DC_OK;
}

/*
 * Function: command_get
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the GET debconf command; retrieves the
 *              value of a given template
 * Assumptions: none
 */
int command_get(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	CHECKARGC(== 1);

	q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
		snprintf(out, outsize, "%u %s",
			CMDSTATUS_SUCCESS, q->value ? q->value : "");
	question_deref(q);

	return DC_OK;
}

/*
 * Function: command_set
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the SET debconf command; sets the value of
 *              a given template
 * Assumptions: none
 */
int command_set(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	int i;
        char *buf;
        size_t bufsize = 1024;

	CHECKARGC(>= 2);

        buf = malloc(bufsize);
        if (!buf)
                return DC_NOTOK;
        memset(buf, 0, bufsize);

	q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		for (i = 2; i <= argc; i++) {
                        while ((strlen(buf) + strlen(argv[i]) + 1) > bufsize) {
                                bufsize += 1024;
                                buf = realloc(buf, bufsize);
                                if (!buf)
                                        return DC_NOTOK;
                        }
			strvacat(buf, bufsize, argv[i], " ", 0);
                }

        /* remove the last space added */
        if (buf[0] != 0)
            buf[strlen(buf)-1] = 0;

		question_setvalue(q, buf);

		if (mod->questions->methods.set(mod->questions, q) != 0)
			snprintf(out, outsize, "%u value set",
				CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot set value",
				CMDSTATUS_INTERNALERROR);
	}
	question_deref(q);

	return DC_OK;
}

/*
 * Function: command_reset
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the RESET debconf command; resets the value of
 *              a given template to the default
 * Assumptions: none
 */
int command_reset(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;

	CHECKARGC(== 1);

	q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		DELETE(q->value);
		q->flags &= ~DC_QFLAG_SEEN;

		if (mod->questions->methods.set(mod->questions, q) != 0)
			snprintf(out, outsize, "%u value reset",
				CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot reset value",
				CMDSTATUS_INTERNALERROR);
	}
	question_deref(q);

	return DC_OK;
}

/*
 * Function: command_subst
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf SUBST command; registers a
 *              substitution variable/value for a template
 * Assumptions: none
 */
int command_subst(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	struct questionvariable;
	char *variable;
	int i;
        char *buf;
        size_t bufsize = 1024;

	CHECKARGC(>= 3);

        buf = malloc(bufsize);
        if (!buf)
                return DC_NOTOK;
        memset(buf, 0, bufsize);

	variable = argv[2];
	q = mod->questions->methods.get(mod->questions, argv[1]);

	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		strvacat(buf, bufsize, argv[3], 0);	
		for (i = 4; i <= argc; i++) {
                        while ((strlen(buf) + strlen(argv[i])) + 1 > bufsize)
                        {
                                bufsize += 1024;
                                buf = realloc(buf, bufsize);
                                if (!buf)
                                        return DC_NOTOK;
                        }

			strvacat(buf, bufsize, " ", argv[i], 0);	
                }
		question_variable_add(q, variable, buf);

		if (mod->questions->methods.set(mod->questions, q) != 0)
			snprintf(out, outsize, "%u variable substituted",
				CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot set variable",
				CMDSTATUS_INTERNALERROR);
	}
	question_deref(q);

	return DC_OK;
}

/*
 * Function: command_register
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf REGISTER command
 * Assumptions: TODO
 */
int command_register(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
        struct question *q;
	CHECKARGC(== 2);

	q = mod->questions->methods.get(mod->questions, argv[2]);
        if (q) {
                /* Duplicate, just add me as an owner as well */
                question_owner_add(q, mod->owner);
                snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
                return DC_OK;
        }
        q = question_new(argv[2]);
        q->template = mod->templates->methods.get(mod->templates, argv[1]);
        template_ref(mod->templates->methods.get(mod->templates, argv[1]));
        mod->questions->methods.set(mod->questions, q);
	snprintf(out, outsize, "%u Question added", CMDSTATUS_SUCCESS);
        return DC_OK;
}

/*
 * Function: command_unregister
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf UNREGISTER command
 * Assumptions: TODO
 */
int command_unregister(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	CHECKARGC(== 1);

	mod->questions->methods.disown(mod->questions, argv[1], mod->owner);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);

	return DC_OK;
}

/*
 * Function: command_purge
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the debconf PURGE command; removes all
 *              questions owned by a given owner
 * Assumptions: none
 */
int command_purge(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	mod->questions->methods.disownall(mod->questions, mod->owner);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

/*
 * Function: command_metaget
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the METAGET debconf command; retrieves given 
 *              attributes for a template
 */
int command_metaget(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	const char *value;

	CHECKARGC(== 2);
	q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		value = question_get_field(q, "", argv[2]);
		snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, value);
	}

	return DC_OK;
}

/*
 * Function: command_fget
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the FGET debconf command; retrieves the value
 *              of a given flag
 * Assumptions: none
 */
int command_fget(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	char *field;

	CHECKARGC(== 2);
	q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		field = argv[2];
		
		/* 
		 * the spec is very vague here, what fields are we supposed 
		 * to recognize?
		 */

		/* isdefault is for backward compability only */
		if (strcmp(field, "seen") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS,
				((q->flags & DC_QFLAG_SEEN) ? "true" : "false"));
                else if (strcmp(field, "isdefault") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS,
				((q->flags & DC_QFLAG_SEEN) ? "false" : "true"));
		else
			snprintf(out, outsize, "%u %s does not exist", CMDSTATUS_BADPARAM, field);

	}
	question_deref(q);

	return DC_OK;
}

/*
 * Function: command_fset
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the FSET debconf command; sets the value of 
 *              the given flag
 * Assumptions: none
 */
int command_fset(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	char *field;

	CHECKARGC(== 3);
	q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		field = argv[2];
		if (strcmp(field, "seen") == 0)
		{
			q->flags &= ~DC_QFLAG_SEEN;
			if (strcmp(argv[3], "true") == 0)
				q->flags |= DC_QFLAG_SEEN;
			mod->questions->methods.set(mod->questions, q);
			snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
		}
		else if (strcmp(field, "isdefault") == 0)
		{
			q->flags &= ~DC_QFLAG_SEEN;
			if (strcmp(argv[3], "false") == 0)
				q->flags |= DC_QFLAG_SEEN;
			mod->questions->methods.set(mod->questions, q);
			snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
		}
		else
			snprintf(out, outsize, "%u %s does not exist", CMDSTATUS_BADPARAM, field);
	}
	question_deref(q);
	return DC_OK;
}

/*
 * Function: command_exist
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the EXIST debconf command; checks to see if a
 *              template exists
 * Assumptions: none
 */
int command_exist(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	CHECKARGC(== 1);

    q = mod->questions->methods.get(mod->questions, argv[1]);
	if (q)
	{
		question_deref(q);
		snprintf(out, outsize, "%u true", CMDSTATUS_SUCCESS);
	}
	else
	{
		snprintf(out, outsize, "%u false", CMDSTATUS_SUCCESS);
	}
	question_deref(q);
	return DC_OK;
}

/*
 * Function: command_stop
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the STOP debconf command; finishes the debconf
 *              session
 * Assumptions: none
 */
int command_stop(struct confmodule *mod, int argc, char **argv,
	char *out, size_t outsize)
{
	CHECKARGC(== 0);
	return DC_OK;
}

/*
 * Function: command_x_loadtemplatefile
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments
 *        char **argv - argument array
 *        char *out - output buffer
 *        size_t outsize - output buffer size
 * Output: int - DC_NOTOK if error, DC_OK otherwise
 * Description: handler for the X_LOADTEMPLATEFILE debconf command; 
 *              loads a new template into the debconf database.  This is a
 *              debian-installer extension.
 * Assumptions: none
 */
int command_x_loadtemplatefile(struct confmodule *mod, int argc, char **argv, 
                               char *out, size_t outsize)
{
    struct template *t = NULL;
    struct question *q = NULL;

    CHECKARGC(== 1);
    t = template_load(argv[1]);
    while (t)
    {
        mod->templates->methods.set(mod->templates, t);
        q = mod->questions->methods.get(mod->questions, t->tag);
        if (q == NULL) {
            q = question_new(t->tag);
            q->template = t;
            template_ref(t);
        }
        mod->questions->methods.set(mod->questions, q);
        t = t->next;
    }
    snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
    return DC_OK;
}
