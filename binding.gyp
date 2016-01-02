{
  "targets": [
    {
      "target_name": "usbscale",
      "sources": [ "src/usbscale.cpp" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
