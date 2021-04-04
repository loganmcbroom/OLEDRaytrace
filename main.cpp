#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <map>
#include <functional>

#include <windows.h>

using vec2 = std::array< float, 2 >;
using vec3 = std::array< float, 3 >;

const float pi = 3.14159265359;
const int screenWidth = 32;
const int screenHeight = 64;
const float uDelta = 0.01;
const float vDelta = 0.01;
const int numFrames = 32;
const float phiDelta = pi / numFrames;
const float K1 = 3; // Screen distance
const float K2 = 8; // Object distance
const float widthScale = 0.8;
const vec3 L = { 0, -2, 1.2 }; // Light position
const int numSurfaces = 1;

int get2DIndex( int x, int y )
    {
    return y * screenWidth + x;
    }

int get2DIndex( vec2 c )
    {
    return get2DIndex( c[0], c[1] );
    }

float dot( const vec3 & a, const vec3 & b )
    {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

float magnitude( float a, float b )
    {
    return sqrt( a * a + b * b );
    }

vec3 normalize( const vec3 & v )
    {
    const float n = sqrt( dot( v, v ) );
    return { v[0] / n, v[1] / n, v[2] / n };
    }

float lightCurve( float x )
    {
    if( x > 0 )
        return 0.3 + 0.7 * x;
    else
        return 0.1 - 0.4 * x;
    }

std::vector<float> generateFrame( float phi, std::function<vec3 (float, float, int)> uv, std::function<vec3 (vec3, int)> normal )
    {
    const float cosPhi = cos( phi );
    const float sinPhi = sin( phi );

    std::vector<float> frameData( screenWidth * screenHeight, 0 );
    std::vector<float> zBuffer( screenWidth * screenHeight, 1000 );

    for( int i = 0; i < numSurfaces; ++i )
        {
        for( float v = -1; v < 1; v += vDelta )
            {
            for( float u = -1; u < 1; u += uDelta )
                {
                // Get basic coordinates
                const vec3 coordsObj = uv( u, v, i );

                // Rotate coordinates by phi around z-axis
                const vec3 coordsObjRot = 
                    {
                    cosPhi * coordsObj[0] - sinPhi * coordsObj[1],
                    sinPhi * coordsObj[0] + cosPhi * coordsObj[1],
                    coordsObj[2]
                    };

                const vec3 coordsObjRotTra =
                    {
                    coordsObjRot[0] + K2, coordsObjRot[1], coordsObjRot[2]
                    };

                const float perspectiveScale = K1 / coordsObjRotTra[0];
                const vec2 screenCoords = // Assuming these fall into [-1,1]
                    {
                    coordsObjRotTra[1] * perspectiveScale,
                    coordsObjRotTra[2] * perspectiveScale,
                    };

                const vec2 pixelCoords =
                    {
                    ( screenCoords[0] / 2.0f + 0.5f ) * screenWidth,
                    ( screenCoords[1] / 2.0f + 0.5f ) * screenHeight,
                    };

                // Consult with z-buffer
                if( zBuffer[ get2DIndex( pixelCoords ) ] < coordsObjRotTra[0] )
                    continue;
                else
                    zBuffer[ get2DIndex( pixelCoords ) ] = coordsObjRotTra[0];
                

                const vec3 VL = normalize( { L[0] - coordsObjRotTra[0], L[1] - coordsObjRotTra[1], L[2] - coordsObjRotTra[2] } ); // Normalized direction towards light from point
                const vec3 N = normal( coordsObj, i ); // Normalized surface normal, unrotated
                const vec3 NR = 
                    {
                    cosPhi * N[0] - sinPhi * N[1],
                    sinPhi * N[0] + cosPhi * N[1],
                    N[2]
                    };
                const float cosTheta = dot( VL, NR );
                const float brightness = std::clamp( lightCurve( cosTheta ), 0.0f, 1.0f );

                frameData[ get2DIndex( pixelCoords ) ] = brightness;
            
                }
            }
        }

    return frameData;
    }

void clearScreen()
  {
  HANDLE                     hStdOut;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD                      count;
  DWORD                      cellCount;
  COORD                      homeCoords = { 0, 0 };

  hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
  if (hStdOut == INVALID_HANDLE_VALUE) return;

  /* Get the number of cells in the current buffer */
  if (!GetConsoleScreenBufferInfo( hStdOut, &csbi )) return;
  cellCount = csbi.dwSize.X *csbi.dwSize.Y;

  /* Fill the entire buffer with spaces */
  if (!FillConsoleOutputCharacter(
    hStdOut,
    (TCHAR) ' ',
    cellCount,
    homeCoords,
    &count
    )) return;

  /* Fill the entire buffer with the current colors and attributes */
  if (!FillConsoleOutputAttribute(
    hStdOut,
    csbi.wAttributes,
    cellCount,
    homeCoords,
    &count
    )) return;

  /* Move the cursor home */
  SetConsoleCursorPosition( hStdOut, homeCoords );
  }

void printFrameToASCII( const std::vector<float> data )
    {
    clearScreen();

    static std::string buffer( ( screenWidth + 1 ) * screenHeight, 'X' );
    static std::string shades( " ,-~:;=!*$@#" );

    std::fill( buffer.begin(), buffer.end(), 0 );

    int dataRead = 0;
    int bufferWrite = buffer.size() - 1;
    for( int y = 0; y < screenHeight; ++y ) 
        {
        buffer[bufferWrite] = '\n';
        --bufferWrite;
        for( int x = 0; x < screenWidth; ++x ) 
            {
            const int shade = floor( data[dataRead] * ( float( shades.length() ) - 0.01f ) );
            buffer[bufferWrite] = shades[ shade ];
            --bufferWrite;
            ++dataRead;
            }
        
        }

    std::cout << buffer;
    }

bool dither( float l, int x, int y )
    {
    static const std::map< float, std::function<bool(int, int)>> ditherMap = 
        {
        { 0.0f,      []( int x, int y ){ return false; } },
        { 0.11f,     []( int x, int y ){ return x % 3 == 0 && y % 3 == 0; } },
        { 0.25f,     []( int x, int y ){ return x % 2 == 0 && y % 2 == 0; } },
        { 0.33f,     []( int x, int y ){ return ( x + y ) % 3 == 0; } },
        { 0.5f,      []( int x, int y ){ return ( x + y ) % 2 == 0; } },
        { 0.66f,     []( int x, int y ){ return ( x + y ) % 3 != 0; } },
        { 0.75f,     []( int x, int y ){ return x % 2 == 0 || y % 2 == 0; } },
        { 0.88f,     []( int x, int y ){ return x % 3 != 0 || y % 3 != 0; } },
        { 1.0f,      []( int x, int y ){ return true; } }
        };

    auto iter = ditherMap.upper_bound( l );
    auto const hi = iter;
    --iter;
    auto const lo = iter;
    if( hi == ditherMap.end() ) return true;

    return ( hi->first - l > l - lo->first ? lo : hi )->second( x, y );
    };

void printFrameToASCII_OLED( const std::vector<float> data )
    {
    clearScreen();

    static std::string buffer( ( screenWidth + 1 ) * screenHeight, 'X' );

    std::fill( buffer.begin(), buffer.end(), 0 );

    int dataRead = 0;
    int bufferWrite = buffer.size() - 1;
    for( int y = 0; y < screenHeight; ++y ) 
        {
        buffer[bufferWrite] = '\n';
        --bufferWrite;

        for( int x = 0; x < screenWidth; ++x ) 
            {
            const bool on = dither( data[dataRead], x, y );
            buffer[bufferWrite] = on? '#' : ' ';
            --bufferWrite;
            ++dataRead;
            }
        }

    std::cout << buffer;
    }

std::vector<char> convertFrameToOLED( const std::vector<float> data )
    {
    std::vector<char> buffer( screenWidth * screenHeight, ' ' );

    int dataRead = 0;
    int bufferWrite = buffer.size() - 1;
    for( int y = 0; y < screenHeight; ++y ) 
        {
        for( int x = 0; x < screenWidth; ++x ) 
            {
            const bool on = dither( data[dataRead], x, y );
            buffer[bufferWrite] = on? '#' : ' ';
            --bufferWrite;
            ++dataRead;
            }
        }

    return buffer;
    }

std::string intToHexString( int i )
    {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << i << " ";
    return ss.str();
    }

std::string convertOLEDStringToHex( std::vector<char> d )
    {
    std::string b;
    for( int x = 0; x < 4; ++x )
        {
        for( int y = 0; y < 128; ++y )
            {
            int byte = 0;
            for( int bit = 0; bit < 8; ++bit )
                {
                if( d[ get2DIndex( x * 8 + bit, y ) ] == '#' ) byte += 1 << bit;
                }
            b += intToHexString( byte );

            b += ", ";
            }
        b += "\n";
        }
    return b;
    }

void printOledDataToFile( std::function<vec3 (float, float, int)> uv, std::function<vec3 (vec3, int)> normal )
    {
    float phi = 0;

    std::ofstream f( "HexDump.txt" );

    for( int i = 0; i < numFrames; ++i )
        {
        phi += phiDelta;
        f << "{";
        f << convertOLEDStringToHex( convertFrameToOLED( generateFrame( phi, uv, normal ) ) );
        f << "},\n";
        }
    }

void renderOledDataToConsole( std::function<vec3 (float, float, int)> uv, std::function<vec3 (vec3, int)> normal )
    {
    float phi = 0;

    while( true )
        {
        phi += phiDelta;
        printFrameToASCII_OLED( generateFrame( phi, uv, normal ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
        }
    }

void renderASCIIDataToConsole( std::function<vec3 (float, float, int)> uv, std::function<vec3 (vec3, int)> normal )
    {
    float phi = 0;

    while( true )
        {
        phi += phiDelta;
        printFrameToASCII( generateFrame( phi, uv, normal ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
        }
    }

//========================================================================================================================================================================
//========================================================================================================================================================================
//========================================================================================================================================================================

vec3 defaultUV( float u, float v, int i )
    {
    const float stripWidth = pi / numSurfaces;
    const float alpha = 2 * stripWidth * i;
    const float r =  widthScale * ( cos( pi * v ) + 1 ) / 2.0f;
    const float theta = 2.0 * pi * v + stripWidth * u / 2.0f + alpha;
    const float x = r * cos( theta );
    const float y = r * sin( theta );
    const float z = v;

    return { x, y, z };
    }

vec3 defaultNormal( vec3 coords, int i )
    {
    if( coords[0] == 0 && coords[1] == 0 ) return vec3( { 0, 0, 0 } );
    return normalize( { coords[0], coords[1], magnitude( coords[0], coords[1] ) * ( widthScale * pi / 2.0f * sin( pi * coords[2] ) ) } );
    }

const float r0 = 1;
const float r1 = .5;

vec3 torusUV( float u, float v, int i )
    {
    const float r = r0 + r1 * cos( pi * v );
    const float theta = pi * u;
    const float y = r * cos( theta );
    const float z = r * sin( theta );
    const float x = r1 * sin( pi * v );

    return { x, y, z };
    }

vec3 torusNormal( vec3 coords, int i )
    {
    const float y = coords[1];
    const float z = coords[2];
    const float theta = atan2( z, y );

    //return normalize( { coords[0] - r0 * cos( theta ), coords[1] - r0 * sin( theta ), coords[2] } );
    return normalize( { coords[0], y - r0 * cos( theta ), z - sin( theta ) } );
    }

int main()
    {
    // Call printOledDataToFile, renderOledDataToConsole, or renderASCIIDataToConsole, passing a uv function and a normal function.
    // The uv funcion will take u and v values on [-1,1] and return xyz coordinates. The integer input can be used for surfaces made of multiple uv sheets.
    // The normal funcion takes xyz coordinates and should return a normalized normal vector to the surface at that point.
    renderASCIIDataToConsole( torusUV, torusNormal );
    }

