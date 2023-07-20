srpm:
	dnf -y install git make gcc flex bison bzip2 xz rpm-build
	git config user.email "fedora@leemhuis.info"
	git config user.name "Thorsten Leemhuis"

	# set UPSTREAMBUILD_GIT_ONLY=0 to include date in %release when building snapshots
	make DISTLOCALVERSION=".vanilla" NO_CONFIGCHECKS=1 UPSTREAMBUILD_GIT_ONLY=0 dist-srpm

	mv redhat/rpm/SRPMS/*.src.rpm  $(outdir)
