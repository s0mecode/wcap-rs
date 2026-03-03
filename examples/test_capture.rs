use std::io::Write;
use std::time::Instant;
use wcap_rs::{CaptureType, Frame, ScreenCapture};

fn main() {
    println!("Initializing Wcap (Rust)...");

    let mut cap = ScreenCapture::new();

    println!("Select capture type:");
    println!("1. Screen");
    println!("2. Window");
    println!("3. Any");
    print!("> ");
    std::io::stdout().flush().unwrap();

    let mut input = String::new();
    std::io::stdin().read_line(&mut input).unwrap();
    let choice: u8 = input.trim().parse().unwrap_or(3);

    let capture_type = match choice {
        1 => CaptureType::Screen,
        2 => CaptureType::Window,
        _ => CaptureType::Any,
    };

    if !cap.select_source(capture_type) {
        eprintln!("Denied.");
        std::process::exit(1);
    }

    println!("Capturing 100 frames...");

    for i in 0..100 {
        let start = Instant::now();

        match cap.get_frame() {
            Some(frame) => {
                let elapsed = start.elapsed().as_millis();

                println!(
                    "Frame {}: {}x{} | Processing Time: {} ms",
                    i, frame.width, frame.height, elapsed
                );

                if i == 99 {
                    save_frame_as_png(&frame, "snapshot.png").expect("Failed to save PNG");
                    println!("Saved snapshot.png");
                }
            }
            None => {
                println!("Stream ended or error.");
                break;
            }
        }
    }

    cap.stop();
}

fn save_frame_as_png(frame: &Frame, path: &str) -> Result<(), Box<dyn std::error::Error>> {
    use image::{ImageBuffer, Rgb};

    let mut rgb_data: Vec<u8> = Vec::with_capacity(frame.data.len());
    for chunk in frame.data.chunks(3) {
        if chunk.len() == 3 {
            rgb_data.extend_from_slice(&[chunk[2], chunk[1], chunk[0]]);
        }
    }

    let img: ImageBuffer<Rgb<u8>, Vec<u8>> =
        ImageBuffer::from_raw(frame.width as u32, frame.height as u32, rgb_data)
            .ok_or("Failed to create image buffer")?;

    img.save(path)?;

    Ok(())
}
