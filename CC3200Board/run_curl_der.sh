curl --tlsv1.2 "https://a62ofxyy56n2n-ats.iot.us-east-2.amazonaws.com:8443/things/CC3200Board/shadow" \
  --cacert AmazonRootCA1.pem \
  --key private.der \
  --cert client.der \
  --cert-type DER --key-type DER

