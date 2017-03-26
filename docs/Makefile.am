EXTRA_DIST = \
conf.py \
changes.rst

SPHINXBUILD = sphinx-build
SPHINXOPTS = -d _build/doctrees $(SPHINXOPTS_EXTRA)

html-local:
	$(SPHINXBUILD) -b html $(SPHINXOPTS) $(srcdir) _build/html

install-html-local: html
	mkdir -p $(DESTDIR)$(htmldir)
	cp -r _build/html $(DESTDIR)$(htmldir)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)

clean-local:
	rm -rf _build
	rm -f ext/refcounting.pyc
