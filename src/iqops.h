//
//  iqops.h
//  Sharp
//
//  Created by Nirav Sheth on 12/22/15.
//
//

#ifndef iqops_h
#define iqops_h

#include <stdio.h>
#include <vips/vips.h>
namespace sharp {
  
  struct Options {
    int position;
    double rotation = 30;
    int indentSize;
    bool useDefaultIndentSize = true;
  };

  
  void GenerateMask(VipsObject* handle, VipsImage *watermarkImage, VipsImage **out, int w, int h, Options *o);
}


#endif /* iqops_h */
