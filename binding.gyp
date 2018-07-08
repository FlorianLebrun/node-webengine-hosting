{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src_cpp/addon.cc",
        "src_cpp/spinlock.cpp",
        "src_cpp/WebxEngine.cc",
        "src_cpp/WebxEngine-js.cc",
        "src_cpp/WebxHttpTransaction.cc",
        "src_cpp/WebxHttpTransaction-js.cc",
        "src_cpp/WebxWebSocketStream.cc",
        "src_cpp/WebxWebSocketStream-js.cc",
        "src_cpp/WebxSession.cc",
        "src_cpp/WebxSession-js.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
