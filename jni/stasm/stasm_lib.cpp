// stasm_lib.cpp: interface to the Stasm library
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"

#if TRACE_IMAGES
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#endif

using namespace stasm;

const char* const stasm_VERSION = STASM_VERSION;

static vec_Mod mods_g;    // the ASM model(s)
static FaceDetector facedet_g; // the face detector

//-----------------------------------------------------------------------------

namespace stasm
{
static Image img_g;     // the current image
static void CheckStasmInit(void)
{
    if (mods_g.empty())
        Err("Models not initialized (missing call to stasm_init?)");
}

static void ShapeToLandmarks( // convert Shape to landmarks (float *)
    float*       landmarks,   // out
    const Shape& shape)       // in
{
    CV_Assert(shape.rows <= stasm_NLANDMARKS);
    int i;
    for (i = 0; i < MIN(shape.rows, stasm_NLANDMARKS); i++)
    {
        landmarks[i * 2]     = float(shape(i, IX));
        landmarks[i * 2 + 1] = float(shape(i, IY));
    }
    // set remaining unused landmarks if any to 0,0
    for (; i < stasm_NLANDMARKS; i++)
    {
        landmarks[i * 2]     = 0;
        landmarks[i * 2 + 1] = 0;
    }
}

static const Shape LandmarksAsShape( // return a Shape
    const float* landmarks)          // in
{
    Shape shape(stasm_NLANDMARKS, 2);
    for (int i = 0; i < stasm_NLANDMARKS; i++)
    {
        shape(i, IX) = landmarks[i*2];
        shape(i, IY) = landmarks[i*2+1];
    }
    return shape;
}

} // namespace stasm

//-----------------------------------------------------------------------------

int stasm_init_ext(        // extended version of stasm_init
    const char* datadir,   // in: directory of face detector files
    int         trace,     // in: 0 normal use, 1 trace to stdout and stasm.log
    void*       detparams) // in: NULL or face detector parameters
{
    int returnval = 1;     // assume success
    CatchOpenCvErrs();
    try
    {
        print_g = (trace != 0);
        trace_g = (trace != 0);
        if (mods_g.empty()) // not yet initialized?
        {
            if (trace)
            {
                // Open a log file in the current directory (if possible).
                // After the log file is opened, lprintf and stasm_printf
                // will print to stasm.log (as well as to stdout).
                OpenLogFile();
            }
            lprintf("Stasm version %s%s\n",
                    stasm_VERSION, trace? "  Logging to stasm.log": "");
            CV_Assert(datadir && datadir[0] && STRNLEN(datadir, SLEN) < SLEN);
            InitMods(mods_g, datadir); // init ASM model(s)
            facedet_g.OpenFaceDetector_(datadir, detparams);
            OpenEyeMouthDetectors(mods_g, datadir);
        }
        CheckStasmInit();
    }
    catch(...)
    {
        returnval = 0; // a call was made to Err or a CV_Assert failed
    }
    UncatchOpenCvErrs();
    return returnval;
}

int stasm_init(            // call once, at bootup (to read models from disk)
    const char* datadir,   // in: directory of face detector files
    int         trace)     // in: 0 normal use, 1 trace to stdout and stasm.log
{
    return stasm_init_ext(datadir, trace, NULL);
}

int stasm_open_image_ext(  // extended version of stasm_open_image
    const char* image,       // in: gray image data, top left corner at 0,0
    int         width,     // in: image width
    int         height,    // in: image height
    const char* imgpath,   // in: image path, used only for err msgs and debug
    int         multiface, // in: 0=return only one face, 1=allow multiple faces
    int         minwidth,  // in: min face width as percentage of img width
    void*       user)      // in: NULL or pointer to user abort func
{
    int returnval = 1;     // assume success
    CatchOpenCvErrs();
    try
    {
        CV_Assert(imgpath && STRNLEN(imgpath, SLEN) < SLEN);
        CV_Assert(multiface == 0 || multiface == 1);
        CV_Assert(minwidth >= 1 && minwidth <= 100);

        CheckStasmInit();

        img_g = Image(height, width,(unsigned char*)image);

#if TRACE_IMAGES
        strcpy(imgpath_g, imgpath); // save the image path (for naming debug images)
#endif
        // call the face detector to detect the face rectangle(s)
        facedet_g.DetectFaces_(img_g, imgpath, multiface == 1, minwidth, user);
    }
    catch(...)
    {
        returnval = 0; // a call was made to Err or a CV_Assert failed
    }
    UncatchOpenCvErrs();
    return returnval;
}

int stasm_open_image(      // call once per image, detect faces
    const char* image,     // in: gray image data, top left corner at 0,0
    int         width,     // in: image width
    int         height,    // in: image height
    const char* imgpath,   // in: image path, used only for err msgs and debug
    int         multiface, // in: 0=return only one face, 1=allow multiple faces
    int         minwidth)  // in: min face width as percentage of img width
{
    return stasm_open_image_ext(image, width, height, imgpath,
                                multiface, minwidth, NULL);
}

int stasm_search_auto_ext( // extended version of stasm_search_auto
    int*   foundface,      // out: 0=no more faces, 1=found face
    float* landmarks,      // out: x0, y0, x1, y1, ..., caller must allocate
    float* estyaw)         // out: NULL or pointer to estimated yaw
{
    int returnval = 1;     // assume success
    *foundface = 0;        // but assume no face found
    CatchOpenCvErrs();
    try
    {
        CheckStasmInit();

        if (img_g.rows == 0 || img_g.cols == 0)
            Err("Image not open (missing call to stasm_open_image?)");

        Shape shape;       // the shape with landmarks
        Image face_roi;    // cropped to area around startshape and possibly rotated
        DetectorParameter detpar_roi; // detpar translated to ROI frame
        DetectorParameter detpar;     // params returned by face det, in img frame

        // Get the start shape for the next face in the image, and the ROI around it.
        // The shape will be wrt the ROI frame.
        if (NextStartShapeAndRoi(shape, face_roi, detpar_roi, detpar,
                                 img_g, mods_g, facedet_g))
        {
            // now working with maybe flipped ROI and start shape in ROI frame
            *foundface = 1;
            if (trace_g)   // show start shape?
                LogShape(RoiShapeToImgFrame(shape, face_roi, detpar_roi, detpar),
                         "auto_start");

            // select an ASM model based on the face's yaw
            const int imod = ABS(EyawAsModIndex(detpar.eyaw, mods_g));

            // do the actual ASM search
            shape = mods_g[imod]->ModSearch_(shape, face_roi);
#if TRACE_IMAGES
            CImage cimg; cvtColor(face_roi, cimg, CV_GRAY2BGR); // color image
            DrawShape(cimg, shape);
            char s[SLEN]; sprintf(s, "%s_90_roishape.bmp", Base(imgpath_g));
            lprintf("%s\n", s);
            if (!cv::imwrite(s, cimg))
                Err("Cannot write %s", s);
#endif
            shape = RoundMat(RoiShapeToImgFrame(shape, face_roi, detpar_roi, detpar));
            // now working with non flipped start shape in image frame
            ShapeToLandmarks(landmarks, shape);
            if (estyaw)
                *estyaw = float(detpar.yaw);
        }
    }
    catch(...)
    {
        returnval = 0; // a call was made to Err or a CV_Assert failed
    }
    UncatchOpenCvErrs();
    return returnval;
}

int stasm_search_auto( // call repeatedly to find all faces
    int*   foundface,  // out: 0=no more faces, 1=found face
    float* landmarks)  // out: x0, y0, x1, y1, ..., caller must allocate
{
    return stasm_search_auto_ext(foundface, landmarks, NULL);
}

int stasm_search_single(   // wrapper for stasm_search_auto and friends
    int*        foundface, // out: 0=no face, 1=found face
    float*      landmarks, // out: x0, y0, x1, y1, ..., caller must allocate
    const char* image,     // in: gray image data, top left corner at 0,0
    int         width,     // in: image width
    int         height,    // in: image height
    const char* imgpath,   // in: image path, used only for err msgs and debug
    const char* datadir)   // in: directory of face detector files
{
    if (!stasm_init(datadir, 0 /*trace*/))
        return false;

    if (!stasm_open_image(image, width, height, imgpath,
                          0 /*multiface*/, 10 /*minwidth*/))
        return false;

    return stasm_search_auto(foundface, landmarks);
}

int stasm_search_pinned(    // call after the user has pinned some points
    float*       landmarks, // out: x0, y0, x1, y1, ..., caller must allocate
    const float* pinned,    // in: pinned landmarks (0,0 points not pinned)
    const char*  image,     // in: gray image data, top left corner at 0,0
    int          width,     // in: image width
    int          height,    // in: image height
    const char*  imgpath)   // in: image path, used only for err msgs and debug
{
    int returnval = 1;     // assume success
    CatchOpenCvErrs();
    try
    {
        CV_Assert(imgpath && STRNLEN(imgpath, SLEN) < SLEN);
        CheckStasmInit();

        img_g = Image(height, width, (unsigned char*)image);

        const Shape pinnedshape(LandmarksAsShape(pinned));

        Shape shape;       // the shape with landmarks
        Image face_roi;    // img cropped to startshape area and maybe rotated
        Shape pinned_roi;  // pinned translated to ROI frame
        DetectorParameter detpar_roi; // detpar translated to ROI frame
        DetectorParameter detpar;     // params returned by pseudo face det, in img frame

        PinnedStartShapeAndRoi(shape, face_roi, detpar_roi, detpar, pinned_roi,
                               img_g, mods_g, pinnedshape);

        // now working with maybe flipped ROI and start shape in ROI frame
        const int imod = ABS(EyawAsModIndex(detpar.eyaw, mods_g));

        shape = mods_g[imod]->ModSearch_(shape, face_roi, &pinned_roi); // ASM search

        shape = RoundMat(RoiShapeToImgFrame(shape, face_roi, detpar_roi, detpar));
        // now working with non flipped start shape in image frame
        ForcePinnedPoints(shape, pinnedshape); // undo above RoundMat on pinned points
        ShapeToLandmarks(landmarks, shape);
        if (trace_g)
            lprintf("\n");
    }
    catch(...)
    {
        returnval = 0; // a call was made to Err or a CV_Assert failed
    }
    UncatchOpenCvErrs();
    return returnval;
}

const char* stasm_lasterr(void) // same as LastErr but not in stasm namespace
{
    return LastErr(); // return the last error message (stashed in sgErr)
}

void stasm_force_points_into_image( // force landmarks into image boundary
    float* landmarks,               // io
    int    ncols,                   // in
    int    nrows)                   // in
{
    for (int i = 0; i < stasm_NLANDMARKS; i++)
    {
        landmarks[i * 2]     = Clamp(landmarks[i * 2],     0.0f, float(ncols-1));
        landmarks[i * 2 + 1] = Clamp(landmarks[i * 2 + 1], 0.0f, float(nrows-1));
    }
}

void stasm_convert_shape( // convert stasm_NLANDMARKS points to given number of points
    float* landmarks,     // io: return all points zero if can't do conversion
    int    nlandmarks)    // in: see ConvertShape
{
    Shape newshape = ConvertShape(LandmarksAsShape(landmarks), nlandmarks);
    if (newshape.rows)
        ShapeToLandmarks(landmarks, newshape);
    else // cannot convert, set all points to 0,0
        for (int i = 0; i < stasm_NLANDMARKS; i++)
            landmarks[i * 2] = landmarks[i * 2 + 1] = 0;
}

void stasm_printf(      // print to stdout and to the stasm.log file if it is open
    const char* format, // args like printf
                ...)
{
    char s[SBIG];
    va_list args;
    va_start(args, format);
    VSPRINTF(s, format, args);
    va_end(args);
    lputs(s);
}
