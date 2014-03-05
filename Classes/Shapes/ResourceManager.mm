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
};

IResourceManager * CreateResourceManager() {
    return new ResourceManager();
}
