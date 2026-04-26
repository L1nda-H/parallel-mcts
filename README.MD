## To Set up GNU Go

run the following to clone Pachi and compile the pure, classic heuristic MCTS mode.
```
git clone --depth 1 https://github.com/pasky/pachi.git
```
```
cd pachi && make DCNN=0 BUILD_KATAGO=0 JOSEKIFIX=0
```

List of [Go Text Protocol commands](https://www.lysator.liu.se/~gunnar/gtp/gtp2-spec-draft2/gtp2-spec.html#SECTION00073000000000000000). Is it outdated? I don't know...