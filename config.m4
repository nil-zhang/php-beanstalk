dnl $Id$
dnl config.m4 for extension beanstalk

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(beanstalk, for beanstalk support,
dnl Make sure that the comment is aligned:
dnl [  --with-beanstalk             Include beanstalk support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(beanstalk, whether to enable beanstalk support,
[  --enable-beanstalk           Enable beanstalk support])

PHP_ARG_WITH(libbeanstalkclient-dir,  for libbeanstalkclient,
[  --with-libbeanstalkclient-dir[=DIR]   Set the path to libbeanstalkclient install prefix.], yes)

if test -z "$PHP_DEBUG"; then
  AC_ARG_ENABLE(debug,
  [  --enable-debug          compile with debugging symbols],[
    PHP_DEBUG=$enableval
  ],[    PHP_DEBUG=no
  ])
fi

if test "$PHP_BEANSTALK" != "no"; then

  if test "$PHP_LIBBEANSTALKCLIENT_DIR" != "no" && test "$PHP_LIBBEANSTALKCLIENT_DIR" != "yes"; then
    if test -r "$PHP_LIBBEANSTALKCLIENT_DIR/include/beanstalkclient.h"; then
      PHP_LIBBEANSTALKCLIENT_DIR="$PHP_LIBBEANSTALKCLIENT_DIR"
    else
      AC_MSG_ERROR([Can't find libbeanstalkclient headers under "$PHP_LIBBEANSTALKCLIENT_DIR"])
    fi
  else
    PHP_LIBBEANSTALKCLIENT_DIR="no"
    for i in /usr /usr/local; do
      if test -r "$i/include/beanstalkclient.h"; then
        PHP_LIBBEANSTALKCLIENT_DIR=$i
        break
      fi
    done
  fi

  AC_MSG_CHECKING([for libbeanstalkclient location])
  if test "$PHP_LIBBEANSTALKCLIENT_DIR" = "no"; then
    AC_MSG_ERROR([beanstalk support requires libbeanstalkclient. Use --with-libbeanstalkclient-dir=<DIR> to specify the prefix where libbeanstalkclient headers and library are located])
  else
    AC_MSG_RESULT([$PHP_LIBBEANSTALKCLIENT_DIR])
    PHP_LIBBEANSTALKCLIENT_INCDIR="$PHP_LIBBEANSTALKCLIENT_DIR/include"
    PHP_ADD_INCLUDE($PHP_LIBBEANSTALKCLIENT_INCDIR)
    PHP_ADD_LIBRARY_WITH_PATH(beanstalkclient, $PHP_LIBBEANSTALKCLIENT_DIR/$PHP_LIBDIR, BEANSTALK_SHARED_LIBADD)

    PHP_SUBST(BEANSTALK_SHARED_LIBADD)

    PHP_BEANSTALK_FILES="beanstalk.c beanstalk_pool.c beanstalk_consistent_hash.c beanstalk_standard_hash.c"

    PHP_NEW_EXTENSION(beanstalk, $PHP_BEANSTALK_FILES, $ext_shared)

    ifdef([PHP_ADD_EXTENSION_DEP],
    [
      PHP_ADD_EXTENSION_DEP(beanstalk, spl, true)
    ])

  fi
fi

  dnl Write more examples of tests here...

  dnl # --with-beanstalk -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/beanstalk.h"  # you most likely want to change this
  dnl if test -r $PHP_BEANSTALK/$SEARCH_FOR; then # path given as parameter
  dnl   BEANSTALK_DIR=$PHP_BEANSTALK
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for beanstalk files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       BEANSTALK_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$BEANSTALK_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the beanstalk distribution])
  dnl fi

  dnl # --with-beanstalk -> add include path
  dnl PHP_ADD_INCLUDE($BEANSTALK_DIR/include)

  dnl # --with-beanstalk -> check for lib and symbol presence
  dnl LIBNAME=beanstalk # you may want to change this
  dnl LIBSYMBOL=beanstalk # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $BEANSTALK_DIR/lib, BEANSTALK_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_BEANSTALKLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong beanstalk lib version or lib not found])
  dnl ],[
  dnl   -L$BEANSTALK_DIR/lib -lm -ldl
  dnl ])
  dnl
  dnl PHP_SUBST(BEANSTALK_SHARED_LIBADD)

