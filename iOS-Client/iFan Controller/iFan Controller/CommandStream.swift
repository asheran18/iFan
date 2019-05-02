//
//  CommandStream.swift
//  iFan Controller
//
//  Created by Brendan Murray on 3/31/19.
//  Copyright © 2019 URI ELE. All rights reserved.
//

import UIKit

class CommandStream: NSObject {
    var inputStream: InputStream!
    var outputStream: OutputStream!
    
    var command = ""
    
    let maxReadLength = 4096
    
    func setupNetworkCommunication(){
        var readStream: Unmanaged<CFReadStream>?
        var writeStream: Unmanaged<CFWriteStream>?
        
        CFStreamCreatePairWithSocketToHost(kCFAllocatorDefault, "131.128.49.175" as CFString, 8080, &readStream, &writeStream)
        
        inputStream = readStream!.takeRetainedValue()
        outputStream = writeStream!.takeRetainedValue()
        
        inputStream.schedule(in: .current, forMode: RunLoop.Mode.common)
        outputStream.schedule(in: .current, forMode: RunLoop.Mode.common)

        inputStream.open()
        outputStream.open()

    }
    
    func sendMessage(message: String) {
        let data = "\(message)".data(using: .ascii)!
        
        _ = data.withUnsafeBytes { outputStream.write($0, maxLength: data.count) }
        
        
    }
    
    func readMessage() -> String {
        let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: 500)
        let tf = inputStream.read(buffer, maxLength: 500)
        if(tf == 0){
            return "Error"
        }
        
        let str = String(cString: buffer)
        return str
    }
    
}
