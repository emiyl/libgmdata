use std::io::Cursor;

use rodio::{Decoder, OutputStream, Sink};

pub struct AudioPlayer {
    stream: Option<OutputStream>,
    sink: Option<Sink>,
    current_bytes: Option<Vec<u8>>,
    current_name: String,
}

impl Default for AudioPlayer {
    fn default() -> Self {
        Self::new()
    }
}

impl AudioPlayer {
    pub fn new() -> Self {
        Self {
            stream: None,
            sink: None,
            current_bytes: None,
            current_name: String::new(),
        }
    }

    fn ensure_sink(&mut self) -> Result<(), String> {
        if self.sink.is_some() {
            return Ok(());
        }

        let (stream, handle) = OutputStream::try_default()
            .map_err(|err| format!("Could not open audio device: {err}"))?;
        let sink = Sink::try_new(&handle)
            .map_err(|err| format!("Could not create audio sink: {err}"))?;

        self.stream = Some(stream);
        self.sink = Some(sink);
        Ok(())
    }

    pub fn load_from_dw(&mut self, dw: &crate::bindings::DataWin, index: usize) -> Result<(), String> {
        if dw.audo.entries.is_null() || dw.audo.count == 0 {
            return Err("No audio entries are available in this file.".to_string());
        }

        let entries = unsafe { std::slice::from_raw_parts(dw.audo.entries, dw.audo.count as usize) };
        let entry = entries
            .get(index)
            .ok_or_else(|| format!("Audio entry {index} is out of range."))?;

        if !entry.present || entry.data.is_null() || entry.dataSize == 0 {
            return Err(format!("Audio entry {index} has no playable data."));
        }

        let bytes = unsafe {
            std::slice::from_raw_parts(entry.data, entry.dataSize as usize)
        }
        .to_vec();

        if let Some(sink) = self.sink.as_ref() {
            sink.stop();
        }

        self.current_bytes = Some(bytes);
        self.current_name = format!("Audio {index}");
        Ok(())
    }

    pub fn play(&mut self) -> Result<(), String> {
        let bytes = self
            .current_bytes
            .clone()
            .ok_or_else(|| "No audio data loaded. Select an audio item first.".to_string())?;

        self.ensure_sink()?;

        if let Some(sink) = self.sink.as_ref() {
            sink.stop();
        }

        let cursor = Cursor::new(bytes);
        let source = Decoder::new(cursor)
            .map_err(|err| format!("Failed to decode audio data: {err}"))?;

        let sink = self.sink.as_ref().expect("sink was just created");
        sink.append(source);
        sink.play();
        Ok(())
    }

    pub fn stop(&mut self) {
        if let Some(sink) = self.sink.as_ref() {
            sink.stop();
        }
    }

    pub fn is_playing(&self) -> bool {
        self.sink
            .as_ref()
            .map(|sink| !sink.empty())
            .unwrap_or(false)
    }

    pub fn current_name(&self) -> &str {
        self.current_name.as_str()
    }

    pub fn bytes_len(&self) -> usize {
        self.current_bytes.as_ref().map_or(0, |bytes| bytes.len())
    }
}
