
void assembleAndSendSpeed(unsigned char newSpeed) {
  data = newSpeed;
  assemble_dcc_msg();
  delay(750);
}

