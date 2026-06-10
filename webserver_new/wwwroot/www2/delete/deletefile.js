 const confirmDelete = async (target) => {

        if(confirm("Are you sure want to remove " + target + "?"))
        {
            try 
    		{
    			const response = await fetch('/uploads/' + target , {
    				method: 'DELETE',
    			})
    			if(response.status == 204)
    			{
    				document.getElementById("divOutput").innerHTML = "Deleted"
    				if(confirm("Deleted OK, reload the page?"))
    					location.reload();
    			}
    			else
    				document.getElementById("divOutput").innerHTML = text
    			
    		}
    		catch (error) {
                console.log(error.message)
    			alert("Fail, exception was thrown")
    		}
        }
    }