# TODO
* support any size chunk
* repo ctl interface
* check scan status
* increment validation
* log client failed part and retry use queue
* authentic
* session cookie
* check system time
* public and private ip address, mac address, system info
* fix self defined toolchain compitible problem with refresh_compile_commands

# final TODO
* kill syncthing
* kill immich
* kill everything
* password manager
* notes generation
* a web system

bazel run @hedron_compile_commands//:refresh_all
bazel run //:refresh_compile_commands
