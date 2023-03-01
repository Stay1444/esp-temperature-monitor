use std::fs::File;
use std::{fs::OpenOptions, sync::Arc};
use std::io::{prelude::*, BufReader};
use std::time::SystemTime;


use chrono::{DateTime, Utc, Date, TimeZone, Duration};
use influxdb::{Client, InfluxDbWriteable};
use rouille::Response;
use serde_json::Value;
use tokio::runtime::Runtime;

#[derive(InfluxDbWriteable)]
struct Reading {
    pub time: DateTime<Utc>,
    pub humidity: i64,
    pub temperature: f64,
}

#[tokio::main]
async fn main() {
    let client = Arc::from(Client::new("http://127.0.0.1:8086", "esp_temp"));
    let mut rt = Runtime::new().unwrap();
    
    rouille::start_server("0.0.0.0:1444", move |request| {
        if request.method() == "POST" && request.url() == "/log" {
            println!("HANDLING REQUEST");

            let mut data = request.data().expect("Oops, body already retrieved, problem \
                                          in the server");

            let mut buf = Vec::new();
            match data.read_to_end(&mut buf) {
                Ok(_) => (),
                Err(_) => return Response::text("Failed to read body")
            };

            let v: Value = serde_json::from_slice(&buf).unwrap();

            println!("Temperature: {}", v["t"]);
            println!("Humidity: {}", v["h"]);

            let c = client.clone();

            rt.block_on(async move {
                handle(&c, v["t"].as_f64().unwrap(), v["h"].as_i64().unwrap(), Utc::now()).await;
            });
            
        }else {
            println!("INVALID REQUEST");
        }
        Response::empty_204()
    });
}

async fn handle(c: &Client, t: f64, h: i64, d: DateTime<Utc>) {
    let reading = Reading {
        time: d,
        temperature: t,
        humidity: h
    };

    let write_result = c
        .query(reading.into_query("esp_data")).await;

    match write_result {
        Ok(s) => {
            println!("Successfully pushed to InfluxDB {}", s);
        },
        Err(s) => {
            println!("Failed to push {}", s)
        },
    }
}
