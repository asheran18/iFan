//
//  graphicsView.swift
//  iFan Controller
//
//  Created by Brendan Murray on 4/27/19.
//  Copyright © 2019 URI ELE. All rights reserved.
//

import UIKit
class graphicsView: UIView {
    
    
    var strokeDistance: CGFloat = 25

    var centerPoint = CGPoint(x: 400/2, y: 800/2)

    var lowerLeft = CGPoint(x: 400/2 - 30, y: 800/2 + 30)

    var lowerRight  = CGPoint(x: 400/2 + 30, y: 800/2 + 30)

    var upperLeft = CGPoint(x: 400/2 - 30, y: 800/2 - 30*2)

    var upperRight = CGPoint(x: 400/2 + 30, y: 800/2 - 30*2)
    
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        // Drawing code
        
        guard let currentContext = UIGraphicsGetCurrentContext() else {
            print("No Context")
            return
        }
        
        drawRectangle(user: currentContext, isFilled: true)
    }

    private func drawRectangle(user context: CGContext, isFilled: Bool) {

    }

    
}
