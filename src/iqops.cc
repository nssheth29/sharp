//
//  iqops.cc
//  Sharp
//
//  Created by Nirav Sheth on 12/22/15.
//
//

#include "common.h"
#include <vips/vips.h>
#include <cmath>
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

  void GenerateMask(VipsObject* handle, VipsImage *watermarkImage, VipsImage **out, int w, int h, Options *o) {
      VipsImage *workingImage = NULL;
      if (o->position == 1) { //north
          vips_embed(watermarkImage, &workingImage, 0, 0, w, h, NULL);
      }
      else if (o->position == 2) {
          vips_embed(watermarkImage, &workingImage, w / 2 - watermarkImage->Xsize / 2, 0, w, h, NULL);
      }
      else if (o->position == 3) {
          vips_embed(watermarkImage, &workingImage, w - watermarkImage->Xsize, 0, w, h, NULL);
      }
      else if (o->position == 4) {
          vips_embed(watermarkImage, &workingImage, 0, h / 2 - watermarkImage->Ysize / 2, w, h, NULL);
      }
      else if (o->position == 5) {
          vips_embed(watermarkImage, &workingImage, w / 2 - watermarkImage->Xsize / 2, h / 2 - watermarkImage->Ysize / 2, w, h, NULL);
      }
      else if (o->position == 6) {
          vips_embed(watermarkImage, &workingImage, w - watermarkImage->Xsize, h / 2 - watermarkImage->Ysize / 2, w, h, NULL);
      }
      else if (o->position == 7) {
          vips_embed(watermarkImage, &workingImage, 0, h - watermarkImage->Ysize, w, h, NULL);
      }
      else if (o->position == 8) {
          vips_embed(watermarkImage, &workingImage, w / 2 - watermarkImage->Xsize / 2, h - watermarkImage->Ysize, w, h, NULL);
      }
      else if (o->position == 9) {
          vips_embed(watermarkImage, &workingImage, w - watermarkImage->Xsize , h - watermarkImage->Ysize, w, h, NULL);
      }
      else if (o->position == 10) {
          horizontalTile(handle, w, h, watermarkImage, &workingImage, true, o);
      }
      else if (o->position == 11) {
          double radians = o->rotation * 180/3.1415926;
          int xPadding = std::abs((watermarkImage->Xsize * cos(radians)));
          int yPadding = std::abs((watermarkImage->Ysize * sin(radians)));
          vips_similarity(watermarkImage, &watermarkImage, "angle", o->rotation, NULL);
          vips_embed(watermarkImage, &watermarkImage, (xPadding + watermarkImage->Xsize) / 2 - watermarkImage->Xsize / 2, (yPadding + watermarkImage->Ysize) / 2 - watermarkImage->Ysize / 2, xPadding + watermarkImage->Xsize, yPadding + watermarkImage->Ysize, NULL);
          horizontalTile(handle, w, h, watermarkImage, &workingImage, false, o);
      }
      vips_object_local(handle, workingImage);
      *out = workingImage;
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

  /* Composite images a and b.
   */
  int CompositeImages( VipsObject *context, VipsImage *a, VipsImage *b, VipsImage **out )
  {
      /* These images will all be unreffed when context is unreffed.
       */
      VipsImage **t = (VipsImage **) vips_object_local_array( context, 16 );
      VipsImage **tt = (VipsImage **) vips_object_local_array(context, 3);
      if (!sharp::HasAlpha(a))
          AddAlphaBand(context, a, &tt[0]);
      else
          tt[0] = a;
      
      if (!sharp::HasAlpha(b))
          AddAlphaBand(context, b, &tt[1]);
      else
          tt[1] = b;
      
      
      /* Extract the alpha channels.
       */
      if( vips_extract_band( tt[0], &t[0], 3, NULL ) ||
         vips_extract_band( tt[1], &t[1], 3, NULL ) )
          return( -1 );
      
      /* Extract the RGB bands.
       */
      if( vips_extract_band( tt[0], &t[2], 0, "n", 3, NULL ) ||
         vips_extract_band( tt[1], &t[3], 0, "n", 3, NULL ) )
          return( -1 );
      
      /* Go to scrgb. This is a simple linear colourspace.
       */
      if( vips_colourspace( t[2], &t[4], VIPS_INTERPRETATION_scRGB, NULL ) ||
         vips_colourspace( t[3], &t[5], VIPS_INTERPRETATION_scRGB, NULL ) )
          return( -1 );
      
      /* Compute normalized input alpha channels.
       */
      if( vips_linear1( t[0], &t[6], 1.0 / 255.0, 0.0, NULL ) ||
         vips_linear1( t[1], &t[7], 1.0 / 255.0, 0.0, NULL ) )
          return( -1 );
      
      // Compute normalized output alpha channel:
      //
      // References:
      // - http://en.wikipedia.org/wiki/Alpha_compositing#Alpha_blending
      // - https://github.com/jcupitt/ruby-vips/issues/28#issuecomment-9014826
      //
      // out_a = src_a + dst_a * (1 - src_a)
      
      if( vips_linear1( t[6], &t[8], -1.0, 1.0, NULL ) ||
         vips_multiply( t[7], t[8], &t[9], NULL ) ||
         vips_add( t[6], t[9], &t[10], NULL ) )
          return( -1 );
      
      // Compute output RGB channels:
      //
      // Wikipedia:
      // out_rgb = (src_rgb * src_a + dst_rgb * dst_a * (1 - src_a)) / out_a
      //
      // `vips_ifthenelse` with `blend=TRUE`: http://bit.ly/1KoSsga
      // out = (cond / 255) * in1 + (1 - cond / 255) * in2
      //
      // Substitutions:
      //
      //     cond --> src_a
      //     in1 --> src_rgb
      //     in2 --> dst_rgb * dst_a (premultiplied destination RGB)
      //
      // Division by `out_a` is not required as `vips_ifthenelse` doesn’t require
      // normalized inputs.
      
      if( vips_multiply( t[5], t[7], &t[11], NULL ) ||
         vips_ifthenelse( t[0], t[4], t[11], &t[12],
                         "blend", TRUE,
                         NULL ) )
          return( -1 );
      
      // back to the start colourspace, unnormalise the alpha, reattach
      if( vips_colourspace( t[12], &t[13], t[1]->Type, NULL ) ||
         vips_linear1( t[10], &t[14], 255.0, 0.0,
                      "uchar", TRUE,
                      NULL ) ||
         vips_bandjoin2( t[13], t[14], &t[15], NULL ) )
          return( -1 );
      
      /* Return a reference to the output image.
       */
      g_object_ref( t[15] );
      *out = t[15];
      return( 0 );
  }
}