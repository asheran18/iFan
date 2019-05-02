//
//  ViewController.swift
//  iFan Controller
//
//  Created by Brendan Murray on 3/31/19.
//  Copyright © 2019 URI ELE. All rights reserved.
//

var dataArray: [String] = []

import UIKit

class mainView: UIViewController, UITextFieldDelegate {
    //let cmdStream = CommandStream()
    var command: String = ""
    //let timer = Timer.scheduledTimer(timeInterval: 1.0, target: self, selector: #selector(fire), userInfo: nil, repeats: true)
    @IBOutlet weak var currTemp: UILabel!
    @IBOutlet weak var FanMode: UILabel!
    @IBOutlet weak var currThr: UILabel!
    @IBOutlet weak var FanUpt: UILabel!
    var count: Int = 0
    override func viewDidLoad() {
        //continues trying to connect

        //cmdStream.setupNetworkCommunication()


        //start timer

        _ = Timer.scheduledTimer(timeInterval: 1.0, target: self, selector: #selector(fire), userInfo: nil, repeats: true)

        
        
    }

    
    @objc func fire()
    {

        
        let dataString = cmdStream.readMessage()
        print(dataString)
        dataArray = dataString.components(separatedBy: ",")
        let argc = dataArray.count
        /* //Debugging Purposes
            print("start")
            for word in dataArray {
                print(word)
            }
            print("stop")
        */
        if(argc > 1){
            currTemp.text = dataArray[1] + "°F"
        }
        
        if(argc > 1){
            if(dataArray[0] == "0"){
                FanMode.text = "OFF"
            } else {
                FanMode.text = "ON"
            }
        }
        
        if(argc > 1){
            let str = dataArray[2]
            let start = str.index(str.startIndex, offsetBy: 0)
            let end = str.index(str.lastIndex(of: ".")!, offsetBy: 2 )
            
            let range = start..<end
            
            let uptime = String(str[range])
            
            FanUpt.text = uptime + " Hours"

            
        }
        
        if(argc > 1){
            if(dataArray[3] == "1000"){
                currThr.text = "None Set"
            } else {
                currThr.text = dataArray[3]
            }
        }
        
    }
    
    @IBAction func toggleFan(_ sender: UIButton) {
        if(FanMode.text == "ON"){
            cmdStream.sendMessage(message: "FAN_OFF")
        } else {
            cmdStream.sendMessage(message: "FAN_AON")
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

    }


    
   // func textFieldShouldReturn(_ textField: UITextField) -> Bool {
       // self.view.endEditing(true)
       // return false
    //}
    
    
}

