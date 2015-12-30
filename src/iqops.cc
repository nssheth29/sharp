//
//  iqops.cc
//  Sharp
//
//  Created by Nirav Sheth on 12/22/15.
//
//

#include <vips/vips.h>
#include <cmath>

#include "common.h"
#include "operations.h"
#include "iqops.h"

namespace sharp {
  void horizontalTile(VipsObject *handle, int rows, int columns, VipsImage *watermarkImage, VipsImage **out, bool indent, Options *o) {
      int possibleColumns = columns / watermarkImage->Ysize;
      int possibleRows = rows / watermarkImage->Xsize;
      int bufferColumns = (columns % watermarkImage->Ysize) / 2;
      VipsImage **t = (VipsImage **) vips_object_local_array( handle, possibleColumns);
      int indentSize = o->useDefaultIndentSize ? watermarkImage->Xsize / 2 : o->indentSize;
      for (int i = 0; i < possibleColumns; i++) {
          if (indent && i % 2 == 0) {
              vips_replicate(watermarkImage, &t[i], possibleRows - 1 , 1, NULL);
              vips_embed(t[i], &t[i], indentSize, 0, rows - indentSize, t[i]->Ysize, NULL);
          }
          else {
              vips_replicate(watermarkImage, &t[i], possibleRows , 1, NULL);
          }
          if (i == 0)
              vips_embed(t[i], &t[i], 0, bufferColumns, t[i]->Xsize, t[i]->Ysize + bufferColumns, NULL);
          else if (i == possibleColumns - 1) {
              vips_embed(t[i], &t[i], 0, 0, t[i]->Xsize, t[i]->Ysize + bufferColumns, NULL);
          }
      }
      int i = 0;
      VipsImage *outImage = t[i++];
      while (i < possibleColumns) {
          vips_join(outImage, t[i++], &outImage, VIPS_DIRECTION_VERTICAL, "expand", true, NULL);
      }
      vips_object_local(handle, outImage);
      int bufferRows = (rows - outImage->Xsize) / 2;
      vips_embed(outImage, &outImage, bufferRows, 0, rows, columns, NULL);
      *out = outImage;
  }
  
  void AddAlphaBand(VipsObject *handle, VipsImage *in, VipsImage **out) {
    // Create single-channel transparency
    VipsImage *black;
    if (vips_black(&black, in->Xsize, in->Ysize, "bands", 1, nullptr)) {
      vips_error_exit( NULL );
    }
    vips_object_local(handle, black);
    
    // Invert to become non-transparent
    VipsImage *alpha;
    if (vips_invert(black, &alpha, nullptr)) {
      vips_error_exit( NULL );
    }
    vips_object_local(handle, alpha);
    
    // Append alpha channel to existing image
    VipsImage *joined;
    if (vips_bandjoin2(in, alpha, &joined, nullptr)) {
      vips_error_exit( NULL );
    }
    vips_object_local(handle, joined);
    *out= joined;
  }


  void GenerateMask(VipsObject* handle, VipsImage *watermarkImage, VipsImage **out, int w, int h, Options *o) {
      VipsImage *workingImage = nullptr;
      VipsImage *alphaImage = watermarkImage;
    
      if (!HasAlpha(watermarkImage)) {
        AddAlphaBand(handle, watermarkImage, &alphaImage);
        vips_object_local(handle, alphaImage);
      }
    
      if (o->position == 1) { //north
          vips_embed(alphaImage, &workingImage, 0, 0, w, h, nullptr);
      }
      else if (o->position == 2) {
          vips_embed(alphaImage, &workingImage, w / 2 - watermarkImage->Xsize / 2, 0, w, h, nullptr);
      }
      else if (o->position == 3) {
          vips_embed(alphaImage, &workingImage, w - watermarkImage->Xsize, 0, w, h, nullptr);
      }
      else if (o->position == 4) {
          vips_embed(alphaImage, &workingImage, 0, h / 2 - watermarkImage->Ysize / 2, w, h, nullptr);
      }
      else if (o->position == 5) {
          vips_embed(alphaImage, &workingImage, w / 2 - watermarkImage->Xsize / 2, h / 2 - watermarkImage->Ysize / 2, w, h, nullptr);
      }
      else if (o->position == 6) {
          vips_embed(alphaImage, &workingImage, w - watermarkImage->Xsize, h / 2 - watermarkImage->Ysize / 2, w, h, NULL);
      }
      else if (o->position == 7) {
          vips_embed(alphaImage, &workingImage, 0, h - watermarkImage->Ysize, w, h, NULL);
      }
      else if (o->position == 8) {
          vips_embed(alphaImage, &workingImage, w / 2 - watermarkImage->Xsize / 2, h - watermarkImage->Ysize, w, h, NULL);
      }
      else if (o->position == 9) {
          vips_embed(alphaImage, &workingImage, w - watermarkImage->Xsize , h - watermarkImage->Ysize, w, h, NULL);
      }
      else if (o->position == 10) {
          horizontalTile(handle, w, h, alphaImage, &workingImage, true, o);
      }
      else if (o->position == 11) {
          double radians = o->rotation * 180/3.1415926;
          int xPadding = std::abs((watermarkImage->Xsize * cos(radians)));
          int yPadding = std::abs((watermarkImage->Ysize * sin(radians)));
          vips_similarity(alphaImage, &alphaImage, "angle", o->rotation, NULL);
          vips_embed(alphaImage, &alphaImage, (xPadding + alphaImage->Xsize) / 2 - alphaImage->Xsize / 2, (yPadding + alphaImage->Ysize) / 2 - alphaImage->Ysize / 2, xPadding + alphaImage->Xsize, yPadding + alphaImage->Ysize, NULL);
          horizontalTile(handle, w, h, alphaImage, &workingImage, false, o);
      }
      vips_object_local(handle, workingImage);
      *out = workingImage;
  }
}