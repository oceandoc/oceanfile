# TODO
// TODO(xieyz) admin user can delete other user and repo, but not permission check other's file


// TODO(xieyz) repo need provide list dir api
// TODO(xieyz) scan and sync add fullly unit_test
// TODO(xieyz) dump to disk only file count over 10 thousand when scan
// TODO(xieyz) use multiple cache bucket when scan
// TODO(xieyz) fix remove files num when scan
// TODO(xieyz) complete sync semantic
// TODO(xieyz) keep user and group and permission when sync
// TODO(xieyz) don't use MurmurHash64A to split thread files when sync
// TODO(xieyz) increment backup, keep as fast as possible with least used disk
// TODO(xieyz) log client failed part and retry
// TODO(xieyz) public and private ip address, mac address, system info


# final TODO
* kill syncthing
* kill immich
* kill everything
* password manager
* notes generation
* a web system

bazel run @hedron_compile_commands//:refresh_all
bazel run //:refresh_compile_commands
