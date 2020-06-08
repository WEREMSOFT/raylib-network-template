# UDP socket template

This is a simple Serverless socket game base app.

Every client listen to the others. Everyone broadcasts a command to the others.

The default command is "status", it's used to send the current position of the character (a cube).

## Building

Run the following command:

```
make run_main &
```

run it several times to have several clients so you can play.

There are some configuration files for VSCode, it can be usefull to have.

There is some boilerplate to compile to html using wasm, but I'm not sure if the socket thing will work.