#ifndef NODL_H_INCLUDED
#define NODL_H_INCLUDED


/* nodl_cap_xxx */

enum nodl_cap
{
  /* { plain, token, full, replay }, per message ? */
  NODL_CAP_SECURITY_SCHEME = 0,
  NODL_CAP_BANO_VERS,
  NODL_CAP_UPDATE_FREQ,
  NODL_CAP_MESSAGING_MODE,
  /* { nrf905, nrf24l01p } */
  NODL_CAP_RF_CHIP,
  NODL_CAP_RF_FREQ,
  NODL_CAP_RF_SPEED,
  NODL_CAP_INVALID
};


#endif /* NODL_H_INCLUDED */
