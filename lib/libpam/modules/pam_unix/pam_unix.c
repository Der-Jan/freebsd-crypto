/*-
 * Copyright 1998 Juniper Networks, Inc.
 * All rights reserved.
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * Portions of this software was developed for the FreeBSD Project by
 * ThinkSec AS and NAI Labs, the Security Research Division of Network
 * Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <login_cap.h>
#include <netdb.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

#include <libutil.h>

#ifdef YP
#include <ypclnt.h>
#endif

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define	PAM_SM_PASSWORD

#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_mod_misc.h>

#define PASSWORD_HASH		"md5"
#define DEFAULT_WARN		(2L * 7L * 86400L)  /* Two weeks */
#define	SALTSIZE		32

static void makesalt(char []);

static char password_hash[] =		PASSWORD_HASH;

enum {
	PAM_OPT_AUTH_AS_SELF	= PAM_OPT_STD_MAX,
	PAM_OPT_NULLOK,
	PAM_OPT_LOCAL_PASS,
	PAM_OPT_NIS_PASS
};

static struct opttab other_options[] = {
	{ "auth_as_self",	PAM_OPT_AUTH_AS_SELF },
	{ "nullok",		PAM_OPT_NULLOK },
	{ "local_pass",		PAM_OPT_LOCAL_PASS },
	{ "nis_pass",		PAM_OPT_NIS_PASS },
	{ NULL, 0 }
};

char *tempname = NULL;

/*
 * authentication management
 */
PAM_EXTERN int
pam_sm_authenticate(pam_handle_t *pamh, int flags __unused,
    int argc, const char *argv[])
{
	login_cap_t *lc;
	struct options options;
	struct passwd *pwd;
	int retval;
	const char *pass, *user, *realpw, *prompt;

	pam_std_option(&options, other_options, argc, argv);

	PAM_LOG("Options processed");

	if (pam_test_option(&options, PAM_OPT_AUTH_AS_SELF, NULL)) {
		pwd = getpwnam(getlogin());
	} else {
		retval = pam_get_user(pamh, &user, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);
		pwd = getpwnam(user);
	}

	PAM_LOG("Got user: %s", user);

	if (pwd != NULL) {
		PAM_LOG("Doing real authentication");
		realpw = pwd->pw_passwd;
		if (realpw[0] == '\0') {
			if (!(flags & PAM_DISALLOW_NULL_AUTHTOK) &&
			    pam_test_option(&options, PAM_OPT_NULLOK, NULL))
				return (PAM_SUCCESS);
			realpw = "*";
		}
		lc = login_getpwclass(pwd);
	} else {
		PAM_LOG("Doing dummy authentication");
		realpw = "*";
		lc = login_getclass(NULL);
	}
	prompt = login_getcapstr(lc, "passwd_prompt", NULL, NULL);
	retval = pam_get_authtok(pamh, PAM_AUTHTOK, &pass, prompt);
	login_close(lc);
	if (retval != PAM_SUCCESS)
		return (retval);
	PAM_LOG("Got password");
	if (strcmp(crypt(pass, realpw), realpw) == 0)
		return (PAM_SUCCESS);

	PAM_VERBOSE_ERROR("UNIX authentication refused");
	return (PAM_AUTH_ERR);
}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_SUCCESS);
}

/*
 * account management
 */
PAM_EXTERN int
pam_sm_acct_mgmt(pam_handle_t *pamh, int flags __unused,
    int argc, const char *argv[])
{
	struct addrinfo hints, *res;
	struct options options;
	struct passwd *pwd;
	struct timeval tp;
	login_cap_t *lc;
	time_t warntime;
	int retval;
	const char *rhost, *tty, *user;
	char rhostip[MAXHOSTNAMELEN];

	pam_std_option(&options, other_options, argc, argv);

	PAM_LOG("Options processed");

	retval = pam_get_user(pamh, &user, NULL);
	if (retval != PAM_SUCCESS)
		return (retval);

	if (user == NULL || (pwd = getpwnam(user)) == NULL)
		return (PAM_SERVICE_ERR);

	PAM_LOG("Got user: %s", user);

	retval = pam_get_item(pamh, PAM_RHOST, (const void **)&rhost);
	if (retval != PAM_SUCCESS)
		return (retval);

	retval = pam_get_item(pamh, PAM_TTY, (const void **)&tty);
	if (retval != PAM_SUCCESS)
		return (retval);

	if (*pwd->pw_passwd == '\0' &&
	    (flags & PAM_DISALLOW_NULL_AUTHTOK) != 0)
		return (PAM_NEW_AUTHTOK_REQD);

	lc = login_getpwclass(pwd);
	if (lc == NULL) {
		PAM_LOG("Unable to get login class for user %s", user);
		return (PAM_SERVICE_ERR);
	}

	PAM_LOG("Got login_cap");

	if (pwd->pw_change || pwd->pw_expire)
		gettimeofday(&tp, NULL);

	/*
	 * Check pw_expire before pw_change - no point in letting the
	 * user change the password on an expired account.
	 */

	if (pwd->pw_expire) {
		warntime = login_getcaptime(lc, "warnexpire",
		    DEFAULT_WARN, DEFAULT_WARN);
		if (tp.tv_sec >= pwd->pw_expire) {
			login_close(lc);
			return (PAM_ACCT_EXPIRED);
		} else if (pwd->pw_expire - tp.tv_sec < warntime &&
		    (flags & PAM_SILENT) == 0) {
			pam_error(pamh, "Warning: your account expires on %s",
			    ctime(&pwd->pw_expire));
		}
	}

	retval = PAM_SUCCESS;
	if (pwd->pw_change) {
		warntime = login_getcaptime(lc, "warnpassword",
		    DEFAULT_WARN, DEFAULT_WARN);
		if (tp.tv_sec >= pwd->pw_change) {
			retval = PAM_NEW_AUTHTOK_REQD;
		} else if (pwd->pw_change - tp.tv_sec < warntime &&
		    (flags & PAM_SILENT) == 0) {
			pam_error(pamh, "Warning: your password expires on %s",
			    ctime(&pwd->pw_change));
		}
	}

	/*
	 * From here on, we must leave retval untouched (unless we
	 * know we're going to fail), because we need to remember
	 * whether we're supposed to return PAM_SUCCESS or
	 * PAM_NEW_AUTHTOK_REQD.
	 */

	if (rhost) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		if (getaddrinfo(rhost, NULL, &hints, &res) == 0) {
			getnameinfo(res->ai_addr, res->ai_addrlen,
			    rhostip, sizeof(rhostip), NULL, 0,
			    NI_NUMERICHOST|NI_WITHSCOPEID);
		}
		if (res != NULL)
			freeaddrinfo(res);
	}

	/*
	 * Check host / tty / time-of-day restrictions
	 */

	if (!auth_hostok(lc, rhost, rhostip) ||
	    !auth_ttyok(lc, tty) ||
	    !auth_timeok(lc, time(NULL)))
		retval = PAM_AUTH_ERR;

	login_close(lc);

	return (retval);
}

/*
 * password management
 *
 * standard Unix and NIS password changing
 */
PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t *pamh, int flags,
    int argc, const char *argv[])
{
#ifdef YP
	struct ypclnt *ypclnt;
	const char *yp_domain, *yp_server;
#endif
	struct options options;
	char salt[SALTSIZE + 1];
	login_cap_t * lc;
	struct passwd *pwd, *old_pwd;
	const char *user, *old_pass, *new_pass;
	char *encrypted;
	int pfd, tfd, retval;

	pam_std_option(&options, other_options, argc, argv);

	PAM_LOG("Options processed");

	if (pam_test_option(&options, PAM_OPT_AUTH_AS_SELF, NULL))
		pwd = getpwnam(getlogin());
	else {
		retval = pam_get_user(pamh, &user, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);
		pwd = getpwnam(user);
	}

	if (pwd == NULL)
		return (PAM_AUTHTOK_RECOVERY_ERR);

	PAM_LOG("Got user: %s", user);

	if (flags & PAM_PRELIM_CHECK) {

		PAM_LOG("PRELIM round");

		if (getuid() == 0 &&
		    (pwd->pw_fields & _PWF_SOURCE) == _PWF_FILES)
			/* root doesn't need the old password */
			return (pam_set_item(pamh, PAM_OLDAUTHTOK, ""));

		if (pwd->pw_passwd[0] == '\0'
		    && pam_test_option(&options, PAM_OPT_NULLOK, NULL)) {
			/*
			 * No password case. XXX Are we giving too much away
			 * by not prompting for a password?
			 * XXX check PAM_DISALLOW_NULL_AUTHTOK
			 */
			old_pass = "";
		} else {
			retval = pam_get_authtok(pamh,
			    PAM_OLDAUTHTOK, &old_pass, NULL);
			if (retval != PAM_SUCCESS)
				return (retval);
		}
		PAM_LOG("Got old password");
		/* always encrypt first */
		encrypted = crypt(old_pass, pwd->pw_passwd);
		if ((old_pass[0] == '\0' && pwd->pw_passwd[0] != '\0') ||
		    strcmp(encrypted, pwd->pw_passwd) != 0)
			return (PAM_PERM_DENIED);
	}
	else if (flags & PAM_UPDATE_AUTHTOK) {
		PAM_LOG("UPDATE round");

		retval = pam_get_authtok(pamh,
		    PAM_OLDAUTHTOK, &old_pass, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);
		PAM_LOG("Got old password");

		/* get new password */
		for (;;) {
			retval = pam_get_authtok(pamh,
			    PAM_AUTHTOK, &new_pass, NULL);
			if (retval != PAM_TRY_AGAIN)
				break;
			pam_error(pamh, "Mismatch; try again, EOF to quit.");
		}
		PAM_LOG("Got new password");
		if (retval != PAM_SUCCESS) {
			PAM_VERBOSE_ERROR("Unable to get new password");
			return (retval);
		}

		if ((old_pwd = pw_dup(pwd)) == NULL)
			return (PAM_BUF_ERR);

		pwd->pw_change = 0;
		lc = login_getclass(NULL);
		if (login_setcryptfmt(lc, password_hash, NULL) == NULL)
			openpam_log(PAM_LOG_ERROR,
			    "can't set password cipher, relying on default");
		login_close(lc);
		makesalt(salt);
		pwd->pw_passwd = crypt(new_pass, salt);
#ifdef YP
		switch (old_pwd->pw_fields & _PWF_SOURCE) {
		case _PWF_FILES:
#endif
			retval = PAM_SERVICE_ERR;
			if (pw_init(NULL, NULL))
				openpam_log(PAM_LOG_ERROR, "pw_init() failed");
			else if ((pfd = pw_lock()) == -1)
				openpam_log(PAM_LOG_ERROR, "pw_lock() failed");
			else if ((tfd = pw_tmp(-1)) == -1)
				openpam_log(PAM_LOG_ERROR, "pw_tmp() failed");
			else if (pw_copy(pfd, tfd, pwd, old_pwd) == -1)
				openpam_log(PAM_LOG_ERROR, "pw_copy() failed");
			else if (pw_mkdb(pwd->pw_name) == -1)
				openpam_log(PAM_LOG_ERROR, "pw_mkdb() failed");
			else
				retval = PAM_SUCCESS;
			pw_fini();
#ifdef YP
			break;
		case _PWF_NIS:
			yp_domain = yp_server = NULL;
			(void)pam_get_data(pamh,
			    "yp_domain", (const void **)&yp_domain);
			(void)pam_get_data(pamh,
			    "yp_server", (const void **)&yp_server);
			ypclnt = ypclnt_new(yp_domain,
			    "passwd.byname", yp_server);
			if (ypclnt == NULL) {
				retval = PAM_BUF_ERR;
			} else if (ypclnt_connect(ypclnt) == -1 ||
			    ypclnt_passwd(ypclnt, pwd, old_pass) == -1) {
				openpam_log(PAM_LOG_ERROR, "%s", ypclnt->error);
				retval = PAM_SERVICE_ERR;
			} else {
				retval = PAM_SUCCESS;
			}
			ypclnt_free(ypclnt);
			break;
		default:
			openpam_log(PAM_LOG_ERROR, "unsupported source 0x%x",
			    pwd->pw_fields & _PWF_SOURCE);
			retval = PAM_SERVICE_ERR;
		}
#endif
		free(old_pwd);
	}
	else {
		/* Very bad juju */
		retval = PAM_ABORT;
		PAM_LOG("Illegal 'flags'");
	}

	return (retval);
}

/* Mostly stolen from passwd(1)'s local_passwd.c - markm */

static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void
to64(char *s, long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}

/* Salt suitable for traditional DES and MD5 */
void
makesalt(char salt[SALTSIZE])
{
	int i;

	/* These are not really random numbers, they are just
	 * numbers that change to thwart construction of a
	 * dictionary. This is exposed to the public.
	 */
	for (i = 0; i < SALTSIZE; i += 4)
		to64(&salt[i], arc4random(), 4);
	salt[SALTSIZE] = '\0';
}

PAM_MODULE_ENTRY("pam_unix");
