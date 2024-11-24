import torch
from transformers import AutoTokenizer, AutoModelForCausalLM


model_name = "openai-community/gpt2"  
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name)

# Set device to GPU if available
device = "cuda" if torch.cuda.is_available() else "cpu"
model.to(device)
# print(device)

# Generate text based on a prompt (same generate_text() function as before)
def generate_text(prompt, max_length=80):
    input_ids = tokenizer.encode(prompt, return_tensors="pt").to(device)
    output = model.generate(input_ids, max_new_tokens = 20, pad_token_id=tokenizer.eos_token_id, no_repeat_ngram_size=3, num_return_sequences=1, top_p = 0.95, temperature = 0.1, do_sample = True)
    generated_text = tokenizer.decode(output[0], skip_special_tokens=True)
    return generated_text

# Example prompt
prompt = """You are a Chatbot capable of answering questions for airline services. Respond the following query without any follow up questions.\n"""

# generated_text = generate_text(prompt)
with open("gpt.txt", "r") as f:
    query = f.readline().rstrip("\n")
    prompt = prompt + query + '\n'
    # print(prompt)
    response = generate_text(prompt)
    
    # Write response to file
    with open("gpt.txt", "w") as f:
        f.write(response)

    # print("Generated response:", response)



# # from transformers import AutoTokenizer, AutoModelForCausalLM
# import torch
# # from transformers import AutoTokenizer, AutoModelForCausalLM

# file_path = "gpt.txt"

# # tokenizer = AutoTokenizer.from_pretrained("openai-community/gpt2")
# # model = AutoModelForCausalLM.from_pretrained("openai-community/gpt2")
# from transformers import GPT2LMHeadModel, AutoTokenizer

# model_name = 'gpt2'
# model = GPT2LMHeadModel.from_pretrained(model_name)
# tokenizer = AutoTokenizer.from_pretrained(model_name, use_fast=False)

# with open(file_path, "r") as f:
#     query = f.readline().rstrip("\n")
#     # print(query)
    
#     # input_ids = tokenizer.encode(query, return_tensors="pt")
#     # output = model.generate(input_ids, 
#     #                     max_length=50, 
#     #                     do_sample=True,
#     #                     temperature=0.9,
#     #                     pad_token_id=tokenizer.eos_token_id,
#     #                     attention_mask=torch.ones(input_ids.shape, dtype=torch.long))
#     # response = tokenizer.decode(output[0], skip_special_tokens=True)
    
#     tokenizer.add_special_tokens({'pad_token': '[PAD]'})
#     input_encoding = tokenizer.encode_plus(
#         query,
#         add_special_tokens=True,
#         max_length=50,
#         padding='longest',
#         truncation=True,
#         return_tensors='pt'
#     )

#     input_ids = input_encoding['input_ids']
#     attention_mask = input_encoding['attention_mask']

#     if input_ids is None or input_ids.numel() == 0:
#         raise ValueError("Failed to tokenize the input text. Please provide a valid input.")

#     # Generate model response
#     output = model.generate(
#         input_ids=input_ids,
#         attention_mask=attention_mask,
#         max_length=50,
#         num_return_sequences=1,  # Set to 1 to get a single response
#         num_beams=5,
#         pad_token_id=tokenizer.pad_token_id,
#         do_sample=True,
#         temperature=0.8,
#         top_k=0,
#         top_p=0.9
#     )

#     response = tokenizer.decode(output[0], skip_special_tokens=True)

#     # Write response to file
#     with open(file_path, "w") as f:
#         f.write(response)

#     print("Generated response:", response)