/*
 * libc/environ/putenv.c
 */

#include <string.h>
#include <malloc.h>
#include <environ.h>

int __put_env(char * str, size_t len, int overwrite)
{
	struct environ_t * xenv = &__xenviron;
	struct environ_t * env;
	struct environ_t * p;

	if(!xenv)
		return -1;

	for(p = xenv->next; p != xenv; p = p->next)
	{
		if(p->content && !strncmp(p->content, str, len))
		{
			if(!overwrite)
			{
				free(str);
			}
			else
			{
				free(p->content);
				p->content = str;
			}
			return 0;
		}
	}

	env = malloc(sizeof(struct environ_t));
	if(!env)
		return -1;

	env->content = str;
	env->prev = xenv->prev;
	env->next = xenv;
	xenv->prev->next = env;
	xenv->prev = env;

	return 0;
}

int putenv(char * str)
{
	char * s;
	const char * e, * z;

	if(!str)
		return -1;

	e = NULL;
	for(z = str; *z; z++)
	{
		if(*z == '=')
			e = z;
	}

	if(!e)
		return -1;

	s = strdup(str);
	if(!s)
		return -1;

	return __put_env(s, e - str, 1);
}
