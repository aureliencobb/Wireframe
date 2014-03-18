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
    
    void LoadPngImage(const string & fileName) {
//        NSString * basePath = [NSString stringWithUTF8String:fileName.c_str()];
//        NSString * resourcePath = [[NSBundle mainBundle] resourcePath];
//        NSString * fullPath = [resourcePath stringByAppendingString:basePath];
//        UIImage * image = [UIImage imageWithContentsOfFile:fullPath];
        UIImage * image = [UIImage imageNamed:[NSString stringWithUTF8String:fileName.c_str()]];
        CGImageRef cgImage = image.CGImage;
        m_imageSize.x = image.size.width;
        m_imageSize.y = image.size.height;
        m_imageData = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
    }
    
    void * GetImageData() {
        return (void *) CFDataGetBytePtr(m_imageData);
    }
    
    ivec2 GetImageSize() {
        return m_imageSize;
    }
    
    void UnloadImage() {
        CFRelease(m_imageData);
    }
    
private:
    CFDataRef m_imageData;
    ivec2 m_imageSize;
};

IResourceManager * CreateResourceManager() {
    return new ResourceManager();
}
