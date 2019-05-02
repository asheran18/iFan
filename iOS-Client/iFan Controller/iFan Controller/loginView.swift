//
//  loginView.swift
//  iFan Controller
//
//  Created by Brendan Murray on 4/26/19.
//  Copyright © 2019 URI ELE. All rights reserved.
//


import UIKit
import Foundation
import CoreGraphics
let cmdStream = CommandStream()
var wrongPWCount = 0
var count = 0;

class loginView: UIViewController {

    
    @IBOutlet weak var pwTxt: UITextField!

    @IBOutlet weak var wrongPW: UILabel!

    @IBOutlet weak var viewLoad: UIView!
    @IBAction func login(_ sender: UIButton) {
        
        if(pwTxt.text == "abcd123"){
            cmdStream.setupNetworkCommunication()
            //cmdStream.setupNetworkCommunication()
            //loading1.startAnimating()
            /*
            while(connected == false && count <= 10){
                count = count+1
                connected = cmdStream.setupNetworkCommunication()
                print("Connecting")
                sleep(1)
            }
            if(count < 10){
                count = 0;
                performSegue(withIdentifier: "seg1", sender: self)
            } else {
                count = 0
                print("Not Connected")
            }
 */
            performSegue(withIdentifier: "seg1", sender: self)
            
        } else {
            wrongPW.isHidden = false
            wrongPWCount = wrongPWCount + 1
            if(wrongPWCount == 3){
                //display error
                    //"Too many login attempts, try again later'
                
                //quit application
                
            }
        }

        //loading1.stopAnimating()
        //loading1.isHidden = true
    }
}
