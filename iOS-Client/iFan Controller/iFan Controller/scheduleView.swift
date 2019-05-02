//
//  scheduleView.swift
//  iFan Controller
//
//  Created by Brendan Murray on 4/1/19.
//  Copyright © 2019 URI ELE. All rights reserved.
//
import UIKit
import Foundation
import CoreGraphics



class scheduleView: UIViewController{

    //let cmdStream1 = CommandStream()
    var command: String = ""
    
    override func viewDidLoad() {
        
        //cmdStream.setupNetworkCommunication()
        
    }
    
    
    @IBOutlet var currSch: UILabel!

    @IBOutlet weak var StartDate: UIDatePicker!
    @IBOutlet weak var EndDate: UIDatePicker!
    @IBOutlet weak var currThr: UILabel!
    @IBOutlet weak var thrInpu: UITextField!
    
    //OPERATIONS
    
    //send data to socket
    @IBAction func SetSched(_ sender: UIButton) {
        let formatter = DateFormatter()
        formatter.dateFormat = "hh:mm"
        
        let formatter1 = DateFormatter()
        formatter1.locale = Locale(identifier: "en_US_POSIX")
        formatter1.dateFormat = "h:mm a"
        formatter1.amSymbol = "AM"
        formatter1.pmSymbol = "PM"
        
        //get data from start/end schedule
        let begin: String = formatter.string(from: StartDate.date)
        let end: String = formatter.string(from: EndDate.date)
        //form string to send
        command = "SET_SCH," + begin + "," + end
        
        let begin1: String = formatter1.string(from: StartDate.date)
        let end1: String = formatter1.string(from: EndDate.date)
        let msg: String = "SET_SCH," + begin + ","+end
        currSch.text = begin1 + " to " + end1
        //send string through socket
        cmdStream.sendMessage(message: msg)

        
    }
    
    @IBAction func ClrSched(_ sender: UIButton) {
        //string = "CLR_SCH"
        command = "CLR_SCH"
        
        //Send socket "string"
        cmdStream.sendMessage(message: command)
        currSch.text = "None Set"
        
    }
    @IBAction func setThr(_ sender: UIButton) {
        let strThr: String = thrInpu.text!
        let thresh = "SET_THR," + strThr
        
        cmdStream.sendMessage(message: thresh)
        currThr.text = strThr + " °F"
        
    }
    //GRAPHICS
        //UI
    

    
    //MISC

    

}

