AUTOMAKE_OPTIONS = foreign
SUBDIRS = src

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = \
	aclocal.m4 compile config.guess config.sub \
	configure depcomp install-sh ltmain.sh     \
	Makefile.in missing

# Helpers for private tarballs generation
PRIVATE_DIST_BACKEND	= xvba
PRIVATE_DIST_PACKAGE	= $(PACKAGE)-$(VERSION).private
PRIVATE_DIST_ARCHIVES	= $(PRIVATE_DIST_PACKAGE).tar.gz

dist.private: $(PRIVATE_DIST_ARCHIVES)

$(PRIVATE_DIST_ARCHIVES):
	@FORCE_DIST=$(FORCE_DIRST) \
	$(MAKE) dist distdir=$(PRIVATE_DIST_PACKAGE) \
	    FORCE_DIST=$${FORCE_DIST:-$(PRIVATE_DIST_BACKEND)} \
	    DIST_ARCHIVES=$(PRIVATE_DIST_ARCHIVES)
