/****************************************************************************
 *                         render.c
 *
 *  This module implements the main raytracing loop.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 * 01/16/92 dfm     Added David Buck's bug fix to add a different offset
 *                      to x and y coordinates for each call to Rand3D() in the
 *                      subsampling routine.  Said to smooth the anti-aliasing.
 *                      Previously, each call returned the same random number,
 *                      hence, no true jittering.
 *                      I consider this an interim fix until we get a better
 *                      algorithm for anti-aliasing.
 *****************************************************************************/

#include "render/render.h"
#include "common/colour.h"
#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "io/dump.h"
#include "render/lighting.h"

extern FileHandle *GLOBAL_outputFileHandle;
extern char Output_File_Name[FILE_NAME_LENGTH];
extern char Input_File_Name[FILE_NAME_LENGTH];
extern char Stat_File_Name[FILE_NAME_LENGTH];
extern char OutputFormat, Color_Bits, PaletteOption;
extern char VerboseFormat;
extern unsigned int Options;
extern int File_Buffer_Size;
extern int Quality;
volatile int Stop_Flag;
extern int First_Line, Last_Line;
extern long Number_Of_Pixels, Number_Of_Rays, Number_Of_Pixels_Supersampled;

extern short *hashTable;
extern unsigned short crctab[256];
#define rand3d(a, b)                                                           \
    crctab[(int)(hashTable[(int)(hashTable[(int)((a)&0xfff)] ^ (b)) &          \
                           0xfff]) &                                           \
           0xff]

Frame GLOBAL_frame;
Ray *VP_Ray;
int Trace_Level, SuperSampleCount;

DBL Max_Trace_Level = 5;
DBL maxclr;

static void check_stats(int y);
static void do_anti_aliasing(int x, int y, RGBAColor *Colour);
static void output_line(int y);

RGBAColor *Previous_Line, *Current_Line;
char *Previous_Line_Antialiased_Flags, *Current_Line_Antialiased_Flags;
Ray ray;

static void
Create_Ray(Ray *ray, int width, int height, DBL x, DBL y)
{
    register DBL X_Scalar, Y_Scalar;
    Vector3D Temp_Vect_1, Temp_Vect_2;

    /* Convert the X Coordinate to be a DBL from 0.0 to 1.0 */
    X_Scalar = (x - (DBL)width / 2.0) / (DBL)width;

    /* Convert the Y Coordinate to be a DBL from 0.0 to 1.0 */
    Y_Scalar =
        (((DBL)(GLOBAL_frame.Screen_Height - 1) - y) - (DBL)height / 2.0) /
        (DBL)height;

    VScale(Temp_Vect_1, GLOBAL_frame.View_Point.Up, Y_Scalar);
    VScale(Temp_Vect_2, GLOBAL_frame.View_Point.Right, X_Scalar);
    VAdd(ray->Direction, Temp_Vect_1, Temp_Vect_2);
    VAdd(ray->Direction, ray->Direction, GLOBAL_frame.View_Point.Direction);
    VNormalize(ray->Direction, ray->Direction);
    ray->InitializeContainers();
    ray->Quadric_Constants_Cached = FALSE;
}

void
Supersample(RGBAColor *result, int x, int y, int Width, int Height)
{
    RGBAColor colour;
    register DBL dx, dy, Jitter_X, Jitter_Y;
    register int Jitt_Offset;
    unsigned char Red, Green, Blue;

    dx = (DBL)x;
    dy = (DBL)y;
    Jitt_Offset = 10;

    Number_Of_Pixels_Supersampled++;

    Make_Colour(result, 0.0, 0.0, 0.0);

    Jitter_X = (rand3d(x + Jitt_Offset, y) & 0x7FFF) / 32768.0 * 0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y) & 0x7FFF) / 32768.0 * 0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, GLOBAL_frame.Screen_Width, GLOBAL_frame.Screen_Height,
        dx + Jitter_X, dy + Jitter_Y);

    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X - 0.3333333,
        dy + Jitter_Y - 0.3333333);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X - 0.3333333, dy + Jitter_Y);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X - 0.3333333,
        dy + Jitter_Y + 0.3333333);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X, dy + Jitter_Y - 0.3333333);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X, dy + Jitter_Y + 0.3333333);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X + 0.3333333,
        dy + Jitter_Y - 0.3333333);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X + 0.3333333, dy + Jitter_Y);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);
    Jitt_Offset += 10;

    Jitter_X = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Jitter_Y = (rand3d(x + Jitt_Offset, y + Jitt_Offset) & 0x7FFF) / 32768.0 *
                   0.33333333 -
               0.16666666;
    Create_Ray(VP_Ray, Width, Height, dx + Jitter_X + 0.3333333,
        dy + Jitter_Y + 0.3333333);
    Trace_Level = 0;
    Trace(VP_Ray, &colour);
    Clip_Colour(&colour, &colour);
    Scale_Colour(&colour, &colour, 0.11111111);
    Add_Colour(result, result, &colour);

    if ((y != First_Line - 1) && (Options & DISPLAY)) {
        Red = (unsigned char)(result->Red * maxclr);
        Green = (unsigned char)(result->Green * maxclr);
        Blue = (unsigned char)(result->Blue * maxclr);
        display_plot(x, y, Red, Green, Blue);
    }
}

void
Read_Rendered_Part()
{
    int rc, x, line_number;
    unsigned char Red, Green, Blue;
    DBL grey;

    maxclr = (DBL)(1 << Color_Bits) - 1.0;
    while ((rc = Read_Line(
                GLOBAL_outputFileHandle, Previous_Line, &line_number)) == 1) {
        if (Options & DISPLAY) {
            for (x = 0; x < GLOBAL_frame.Screen_Width; x++) {
                if (PaletteOption == GREY) {
                    grey = Previous_Line[x].Red * 0.287 +
                           Previous_Line[x].Green * 0.589 +
                           Previous_Line[x].Blue * 0.114;
                    Red = Green = Blue = (unsigned char)(grey * maxclr);
                } else {
                    Red = (unsigned char)(Previous_Line[x].Red * maxclr);
                    Green = (unsigned char)(Previous_Line[x].Green * maxclr);
                    Blue = (unsigned char)(Previous_Line[x].Blue * maxclr);
                }
                display_plot(x, line_number, Red, Green, Blue);
                COOPERATE /* Moved inside loop JLN 12/91 */
            }
        }
    }

    First_Line = line_number + 1;

    if (rc == 0) {
        Close_File(GLOBAL_outputFileHandle);
        if (Open_File(GLOBAL_outputFileHandle, Output_File_Name,
                &GLOBAL_frame.Screen_Width, &GLOBAL_frame.Screen_Height,
                File_Buffer_Size, APPEND_MODE) != 1) {
            fprintf(stderr, "Error opening output file\n");
            exit(1);
        }
        return;
    }

    fprintf(stderr, "Error reading aborted data file\n");
}

void
Start_Tracing()
{
    RGBAColor Colour;
    register int x, y;
    unsigned char Red, Green, Blue;
    DBL grey;

    for (y = (Options & ANTIALIAS) ? First_Line - 1 : First_Line; y < Last_Line;
         y++) {

        check_stats(y);

        for (x = 0; x < GLOBAL_frame.Screen_Width; x++) {

            TEST_ABORT

            if (Stop_Flag) {
                close_all();
                PRINT_STATS
                /* exit with error if image not completed/user abort*/
                exit(2);
            }

            Number_Of_Pixels++;

            Create_Ray(VP_Ray, GLOBAL_frame.Screen_Width,
                GLOBAL_frame.Screen_Height, (DBL)x, (DBL)y);
            Trace_Level = 0;
            Trace(&ray, &Colour);
            Clip_Colour(&Colour, &Colour);

            Current_Line[x] = Colour;

            if (Options & ANTIALIAS) {
                do_anti_aliasing(x, y, &Colour);
            }

            if (y != First_Line - 1) {
                if (PaletteOption == GREY) {
                    grey = Previous_Line[x].Red * 0.287 +
                           Previous_Line[x].Green * 0.589 +
                           Previous_Line[x].Blue * 0.114;
                    Red = Green = Blue = (unsigned char)(grey * maxclr);
                } else {
                    Red = (unsigned char)(Colour.Red * maxclr);
                    Green = (unsigned char)(Colour.Green * maxclr);
                    Blue = (unsigned char)(Colour.Blue * maxclr);
                }
                if (Options & DISPLAY) {
                    display_plot(x, y, Red, Green, Blue);
                }
            }
        }
        output_line(y);
    }

    if (Options & DISKWRITE) {
        Write_Line(GLOBAL_outputFileHandle, Previous_Line, Last_Line - 1);
    }
}

static void
check_stats(register int y)
{
    FILE *stat_file;

    /* New verbose options CdW */
    if (Options & VERBOSE && VerboseFormat == '0') {
        printf("POV-Ray rendering %s to %s", Input_File_Name, Output_File_Name);
        if ((First_Line != 0) || (Last_Line != GLOBAL_frame.Screen_Height)) {
            printf(" from %4d to %4d:\n", First_Line, Last_Line);
        } else {
            printf(":\n");
        }
        printf("Res %4d X %4d. Calc line %4d of %4d", GLOBAL_frame.Screen_Width,
            GLOBAL_frame.Screen_Height, (y - First_Line) + 1,
            Last_Line - First_Line);
        if (!(Options & ANTIALIAS)) {
            printf(".");
        }
    }
    if (Options & VERBOSE_FILE) {
        stat_file = fopen(Stat_File_Name, "w+t");
        fprintf(stat_file, "Line %4d.\n", y);
        fclose(stat_file);
    }

    /* Use -vO for Old style verbose */
    if (Options & VERBOSE && (VerboseFormat == 'O')) {
        printf("Line %4d", y);
    }
    if (Options & VERBOSE && VerboseFormat == '1') {
        fprintf(stderr, "Res %4d X %4d. Calc line %4d of %4d",
            GLOBAL_frame.Screen_Width, GLOBAL_frame.Screen_Height,
            (y - First_Line) + 1, Last_Line - First_Line);
        if (!(Options & ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }

    if (Options & ANTIALIAS) {
        SuperSampleCount = 0;
    }
}

static void
do_anti_aliasing(register int x, register int y, RGBAColor *Colour)
{
    char Antialias_Center_Flag = 0;

    Current_Line_Antialiased_Flags[x] = 0;

    if (x != 0) {
        if (Colour_Distance(&Current_Line[x - 1], &Current_Line[x]) >=
            GLOBAL_frame.Antialias_Threshold) {
            Antialias_Center_Flag = 1;
            if (!(Current_Line_Antialiased_Flags[x - 1])) {
                Supersample(&Current_Line[x - 1], x - 1, y,
                    GLOBAL_frame.Screen_Width, GLOBAL_frame.Screen_Height);
                Current_Line_Antialiased_Flags[x - 1] = 1;
                SuperSampleCount++;
            }
        }
    }

    if (y != First_Line - 1) {
        if (Colour_Distance(&Previous_Line[x], &Current_Line[x]) >=
            GLOBAL_frame.Antialias_Threshold) {
            Antialias_Center_Flag = 1;
            if (!(Previous_Line_Antialiased_Flags[x])) {
                Supersample(&Previous_Line[x], x, y - 1,
                    GLOBAL_frame.Screen_Width, GLOBAL_frame.Screen_Height);
                Previous_Line_Antialiased_Flags[x] = 1;
                SuperSampleCount++;
            }
        }
    }

    if (Antialias_Center_Flag) {
        Supersample(&Current_Line[x], x, y, GLOBAL_frame.Screen_Width,
            GLOBAL_frame.Screen_Height);
        Current_Line_Antialiased_Flags[x] = 1;
        *Colour = Current_Line[x];
        SuperSampleCount++;
    }
}

void
Initialize_Renderer()
{
    register int i;

    VP_Ray = &ray;
    maxclr = (DBL)(1 << Color_Bits) - 1.0;

    Previous_Line = new RGBAColor[(GLOBAL_frame.Screen_Width + 1)];
    Current_Line = new RGBAColor[(GLOBAL_frame.Screen_Width + 1)];

    for (i = 0; i <= GLOBAL_frame.Screen_Width; i++) {
        Previous_Line[i].Red = 0.0;
        Previous_Line[i].Green = 0.0;
        Previous_Line[i].Blue = 0.0;

        Current_Line[i].Red = 0.0;
        Current_Line[i].Green = 0.0;
        Current_Line[i].Blue = 0.0;
    }

    if (Options & ANTIALIAS) {
        Previous_Line_Antialiased_Flags =
            new char[(GLOBAL_frame.Screen_Width + 1)];
        Current_Line_Antialiased_Flags =
            new char[(GLOBAL_frame.Screen_Width + 1)];

        for (i = 0; i <= GLOBAL_frame.Screen_Width; i++) {
            (Previous_Line_Antialiased_Flags)[i] = 0;
            (Current_Line_Antialiased_Flags)[i] = 0;
        }
    }

    ray.Initial = GLOBAL_frame.View_Point.Location;
}

static void
output_line(register int y)
{
    RGBAColor *Temp_Colour_Ptr;
    char *Temp_Char_Ptr;

    if (Options & DISKWRITE) {
        if (y > First_Line) {
            Write_Line(GLOBAL_outputFileHandle, Previous_Line, y - 1);
        }
    }

    if (Options & VERBOSE) {
        if (Options & ANTIALIAS && VerboseFormat != '1') {
            printf(" supersampled %d times.", SuperSampleCount);
        }

        if (Options & ANTIALIAS && VerboseFormat == '1') {
            fprintf(stderr, " supersampled %d times.", SuperSampleCount);
        }
        if (VerboseFormat == '1') {
            fprintf(stderr, "\r");
        } else {
            fprintf(stderr, "\n");
        }
    }
    Temp_Colour_Ptr = Previous_Line;
    Previous_Line = Current_Line;
    Current_Line = Temp_Colour_Ptr;

    Temp_Char_Ptr = Previous_Line_Antialiased_Flags;
    Previous_Line_Antialiased_Flags = Current_Line_Antialiased_Flags;
    Current_Line_Antialiased_Flags = Temp_Char_Ptr;
}

void
Trace(Ray *Ray, RGBAColor *Colour)
{
    SimpleBody *Object;
    Intersection *Local_Intersection, *New_Intersection;
    register int Intersection_Found;

    COOPERATE
    Number_Of_Rays++;
    Make_Colour(Colour, 0.0, 0.0, 0.0);

    Intersection_Found = FALSE;
    Local_Intersection = NULL;

    if (Trace_Level > (int)Max_Trace_Level) {
        return;
    }

    if (GLOBAL_frame.Fog_Distance == 0.0) {
        Make_Colour(Colour, 0.0, 0.0, 0.0);
    } else {
        *Colour = GLOBAL_frame.Fog_Colour;
    }

    if (Options & DEBUGGING) {
        printf("Calculating intersections level %d\n", Trace_Level);
    }

    /* What objects does this ray intersect? */
    for (Object = GLOBAL_frame.Objects; Object != NULL;
         Object = Object->Next_Object) {
        COOPERATE
        if ((New_Intersection = Intersection(Object, Ray)) != NULL) {
            if (Intersection_Found) {
                if (Local_Intersection->Depth > New_Intersection->Depth) {
                    delete Local_Intersection;
                    Local_Intersection = New_Intersection;
                } else {
                    delete New_Intersection;
                }
            } else {
                Local_Intersection = New_Intersection;
            }

            Intersection_Found = TRUE;
        }
    }

    if (Intersection_Found) {
        Determine_Surface_Colour(Local_Intersection, Colour, Ray, FALSE);
        delete Local_Intersection;
    }
}
