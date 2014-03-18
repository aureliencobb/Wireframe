//
//  ResourceManager.mm
//  ModelViewer
//
//  Created by Aurelien Cobb on 24/02/2014.
//
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <string>
#import <iostream>
#import "Interfaces.hpp"

using namespace std;

class ResourceManager : public IResourceManager {
public:
    string GetResourcepath() const {
        NSString * bundlePath = [[NSBundle mainBundle] resourcePath];
        return [bundlePath UTF8String];
    }
    
    TextureDescription LoadPngImage(const string & fileName) {
        UIImage * image = [UIImage imageNamed:[NSString stringWithUTF8String:fileName.c_str()]];
        if (!image) {
            NSString * basePath = [NSString stringWithUTF8String:fileName.c_str()];
            NSString * resourcePath = [[NSBundle mainBundle] resourcePath];
            NSString * fullPath = [resourcePath stringByAppendingString:basePath];
            image = [UIImage imageWithContentsOfFile:fullPath];
        }
        CGImageRef cgImage = image.CGImage;
        m_imageData = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
        
        TextureDescription description;
        description.size.x = CGImageGetWidth(cgImage);
        description.size.y = CGImageGetHeight(cgImage);
        bool hasAlpha = CGImageGetAlphaInfo(cgImage) != kCGImageAlphaNone;
        CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
        switch (CGColorSpaceGetModel(colorSpace)) {
            case kCGColorSpaceModelMonochrome:
                description.Format = hasAlpha ? TextureFormatGreyAlpha : TextureFormatGrey;
                break;
            case kCGColorSpaceModelRGB:
                description.Format = hasAlpha ? TextureFormatRGBA : TextureFormatRGB;
                break;
            default:
                assert(!"Unsupported color space");
                break;
        }
        description.BitsPerComponent = CGImageGetBitsPerComponent(cgImage);
        return description;
    }
    
    void * GetImageData() {
        return (void *) CFDataGetBytePtr(m_imageData);
    }
    
    void UnloadImage() {
        CFRelease(m_imageData);
    }
    
private:
    CFDataRef m_imageData;
};

IResourceManager * CreateResourceManager() {
    return new ResourceManager();
}
