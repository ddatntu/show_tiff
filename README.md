# show_tiff

A simple utility to show a tiff image, or directory of tiff images, in X windows.

This is particularly useful when viewing lots of numbered files (e.g. medical images), as it can cycle through them with a starting number and ending number. It will also, if it can not find the next image in the sequence (or if no ending number is specified) automatically loop back to the start. The file pattern can also be altered to account for different text and number sequences. To move onto each new image in sequence press any key apart from 'q', to quit the application either mouse click on the close icon or press the 'q' key.

```
Usage: ./show_tiff [-q] [-e] [-s start] [-f file_pattern] [-d directory_path]
  -q                do not restart at end of images
  -e                debug output
  -s start          starting number (default: 0)
  -m minimum        minimum number (default: 0)
  -x maximum        maximum number (default: no maximum)
  -f file_pattern   (default: 'FRONT%04i.tif')
  -d directory_path (default: '')
```
